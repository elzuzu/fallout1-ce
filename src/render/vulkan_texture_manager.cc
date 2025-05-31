#include "render/vulkan_texture_manager.h"
#include "render/fallout_memory_manager.h"
#include "render/vulkan_thread_manager.h"
#include <cmath>

#include <SDL.h>
#include <cstring>
#include <vector>

namespace fallout {

bool VulkanTextureManager::init(VkPhysicalDevice physicalDevice, VkDevice device,
    FalloutMemoryManager* memoryManager, VkCommandPool commandPool, VkQueue queue)
{
    m_physicalDevice = physicalDevice;
    m_device = device;
    m_memoryManager = memoryManager;
    m_commandPool = commandPool;
    m_queue = queue;
    return true;
}

void VulkanTextureManager::destroy()
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    for (auto& it : textureCache) {
        if (it.second.view != VK_NULL_HANDLE)
            vkDestroyImageView(m_device, it.second.view, nullptr);
        if (it.second.image != VK_NULL_HANDLE)
            vmaDestroyImage(m_memoryManager ? m_memoryManager->getAllocator() : VK_NULL_HANDLE,
                it.second.image, it.second.memory);
    }
    textureCache.clear();
}

void VulkanTextureManager::loadTextureAsync(const std::string& path)
{
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        if (textureCache.find(path) != textureCache.end())
            return;
    }

    gVulkanThread.submit({[this, path]() {
        Texture tex{};
        if (loadTexture(path, tex)) {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            textureCache[path] = tex;
        }
    }});
}

const VulkanTextureManager::Texture* VulkanTextureManager::getTexture(const std::string& path) const
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = textureCache.find(path);
    if (it == textureCache.end())
        return nullptr;
    return &it->second;
}

bool VulkanTextureManager::loadTexture(const std::string& path, Texture& out)
{
    // TODO: Proper image loading. Currently loads BMP via SDL as a placeholder.
    SDL_Surface* surface = SDL_LoadBMP(path.c_str());
    if (surface == nullptr)
        return false;

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    int32_t width = surface->w;
    int32_t height = surface->h;
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * 4);

    std::vector<unsigned char> pixels(imageSize);
    SDL_LockSurface(surface);
    memcpy(pixels.data(), surface->pixels, imageSize);
    SDL_UnlockSurface(surface);

    SDL_FreeSurface(surface);

    out.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = out.mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VulkanImage img = m_memoryManager->createSpriteAtlas(static_cast<uint32_t>(width),
        static_cast<uint32_t>(height), pixels.data());
    if (img.image == VK_NULL_HANDLE)
        return false;
    out.image = img.image;
    out.view = img.imageView;
    out.memory = img.allocation;


    out.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

    generateMipmaps(out, format, width, height);

    return true;
}

void VulkanTextureManager::generateMipmaps(Texture& texture, VkFormat format,
    int32_t texWidth, int32_t texHeight)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        return;

    VkCommandBufferAllocateInfo cmdInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmdInfo.commandPool = m_commandPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_device, &cmdInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.image = texture.image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < texture.mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        vkCmdBlitImage(cmd, texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = texture.mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(m_queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_queue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
}

} // namespace fallout
