#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>

struct FalloutFrame {
    uint32_t width;
    uint32_t height;
    int16_t offsetX;
    int16_t offsetY;
    std::vector<uint8_t> pixelData;
};

struct FalloutSprite {
    uint32_t frameCount;
    uint32_t framesPerDirection;
    uint32_t directions;
    std::vector<FalloutFrame> frames;
    float frameDuration = 0.2f;
    bool loops = true;
    VkImage textureAtlas = VK_NULL_HANDLE;
    VkImageView textureView = VK_NULL_HANDLE;
    VkDeviceMemory textureMemory = VK_NULL_HANDLE;
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    std::vector<glm::vec4> frameUVs;
};

struct FalloutPalette {
    std::array<glm::u8vec3, 256> colors{};
    uint32_t transparentIndex = 0;
    VkImage paletteTexture = VK_NULL_HANDLE;
    VkImageView paletteView = VK_NULL_HANDLE;
    VkDeviceMemory paletteMemory = VK_NULL_HANDLE;
};

class FalloutAssetLoader {
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;
    VkCommandPool transferCommandPool = VK_NULL_HANDLE;

    std::unordered_map<std::string, std::unique_ptr<FalloutSprite>> spriteCache;
    std::unordered_map<std::string, std::unique_ptr<FalloutPalette>> paletteCache;

public:
    bool initialize(VkDevice dev, VmaAllocator alloc, VkQueue queue, VkCommandPool cmdPool);
    std::shared_ptr<FalloutSprite> loadSprite(const std::string& filename, const std::string& paletteName = "default");
    std::shared_ptr<FalloutPalette> loadPalette(const std::string& filename, const std::string& name);

private:
    bool loadFRMFile(const std::string& filename, FalloutSprite& sprite);
    bool loadPALFile(const std::string& filename, FalloutPalette& palette);
    bool createSpriteAtlas(FalloutSprite& sprite, const std::string& paletteName);
    bool createPaletteTexture(FalloutPalette& palette);
    bool createVulkanTexture(const void* data, uint32_t width, uint32_t height,
                             VkImage& image, VkImageView& imageView, VkDeviceMemory& memory,
                             VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
};
bool FalloutAssetLoader::initialize(VkDevice dev, VmaAllocator alloc, VkQueue queue, VkCommandPool cmdPool)
{
    device = dev;
    allocator = alloc;
    transferQueue = queue;
    transferCommandPool = cmdPool;
    if (!loadPalette("master.pal", "default")) {
        return false;
    }
    return true;
}

std::shared_ptr<FalloutSprite> FalloutAssetLoader::loadSprite(const std::string& filename, const std::string& paletteName)
{
    auto it = spriteCache.find(filename);
    if (it != spriteCache.end())
        return it->second;

    auto sprite = std::make_unique<FalloutSprite>();
    if (!loadFRMFile(filename, *sprite))
        return nullptr;
    if (!createSpriteAtlas(*sprite, paletteName))
        return nullptr;

    auto result = sprite.get();
    spriteCache[filename] = std::move(sprite);
    return result;
}

std::shared_ptr<FalloutPalette> FalloutAssetLoader::loadPalette(const std::string& filename, const std::string& name)
{
    auto palette = std::make_unique<FalloutPalette>();
    if (!loadPALFile(filename, *palette))
        return nullptr;
    if (!createPaletteTexture(*palette))
        return nullptr;
    auto result = palette.get();
    paletteCache[name] = std::move(palette);
    return result;
}

bool FalloutAssetLoader::loadFRMFile(const std::string& filename, FalloutSprite& sprite)
{
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file)
        return false;

    uint32_t version;
    fread(&version, 4, 1, file);
    uint16_t fps;
    fread(&fps, 2, 1, file);
    sprite.frameDuration = fps > 0 ? 1.0f / fps : 0.2f;
    uint16_t actionFrame;
    fread(&actionFrame, 2, 1, file);
    fread(&sprite.framesPerDirection, 2, 1, file);
    fread(&sprite.directions, 2, 1, file);
    sprite.frameCount = sprite.framesPerDirection * sprite.directions;
    sprite.frames.resize(sprite.frameCount);
    fseek(file, 24, SEEK_CUR);
    std::vector<uint32_t> frameOffsets(sprite.frameCount);
    fread(frameOffsets.data(), 4, sprite.frameCount, file);
    for (uint32_t i = 0; i < sprite.frameCount; ++i) {
        fseek(file, frameOffsets[i], SEEK_SET);
        FalloutFrame& frame = sprite.frames[i];
        fread(&frame.width, 2, 1, file);
        fread(&frame.height, 2, 1, file);
        fread(&frame.offsetX, 2, 1, file);
        fread(&frame.offsetY, 2, 1, file);
        uint32_t count = frame.width * frame.height;
        frame.pixelData.resize(count);
        fread(frame.pixelData.data(), 1, count, file);
    }
    fclose(file);
    return true;
}

bool FalloutAssetLoader::loadPALFile(const std::string& filename, FalloutPalette& palette)
{
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file)
        return false;

    for (int i = 0; i < 256; ++i) {
        uint8_t r, g, b;
        fread(&r, 1, 1, file);
        fread(&g, 1, 1, file);
        fread(&b, 1, 1, file);
        palette.colors[i] = glm::u8vec3(r, g, b);
    }
    fclose(file);
    return true;
}

bool FalloutAssetLoader::createSpriteAtlas(FalloutSprite& sprite, const std::string& paletteName)
{
    auto palIt = paletteCache.find(paletteName);
    if (palIt == paletteCache.end())
        return false;
    auto& palette = *palIt->second;

    uint32_t maxW = 0, maxH = 0;
    for (const auto& frame : sprite.frames) {
        maxW = std::max(maxW, frame.width);
        maxH = std::max(maxH, frame.height);
    }
    uint32_t framesPerRow = static_cast<uint32_t>(std::ceil(std::sqrt(sprite.frameCount)));
    sprite.atlasWidth = framesPerRow * maxW;
    sprite.atlasHeight = ((sprite.frameCount + framesPerRow - 1) / framesPerRow) * maxH;

    std::vector<uint8_t> atlas(sprite.atlasWidth * sprite.atlasHeight * 4, 0);
    sprite.frameUVs.resize(sprite.frameCount);

    for (uint32_t i = 0; i < sprite.frameCount; ++i) {
        const auto& frame = sprite.frames[i];
        uint32_t atlasX = (i % framesPerRow) * maxW;
        uint32_t atlasY = (i / framesPerRow) * maxH;
        for (uint32_t y = 0; y < frame.height; ++y) {
            for (uint32_t x = 0; x < frame.width; ++x) {
                uint32_t src = y * frame.width + x;
                uint32_t dst = ((atlasY + y) * sprite.atlasWidth + (atlasX + x)) * 4;
                uint8_t pi = frame.pixelData[src];
                glm::u8vec3 c = palette.colors[pi];
                atlas[dst + 0] = c.r;
                atlas[dst + 1] = c.g;
                atlas[dst + 2] = c.b;
                atlas[dst + 3] = (pi == palette.transparentIndex) ? 0 : 255;
            }
        }
        sprite.frameUVs[i] = glm::vec4(
            float(atlasX) / sprite.atlasWidth,
            float(atlasY) / sprite.atlasHeight,
            float(frame.width) / sprite.atlasWidth,
            float(frame.height) / sprite.atlasHeight);
    }
    return createVulkanTexture(atlas.data(), sprite.atlasWidth, sprite.atlasHeight,
                               sprite.textureAtlas, sprite.textureView, sprite.textureMemory);
}

bool FalloutAssetLoader::createPaletteTexture(FalloutPalette& palette)
{
    std::vector<uint8_t> palData(256 * 3);
    for (int i = 0; i < 256; ++i) {
        palData[i * 3 + 0] = palette.colors[i].r;
        palData[i * 3 + 1] = palette.colors[i].g;
        palData[i * 3 + 2] = palette.colors[i].b;
    }
    return createVulkanTexture(palData.data(), 256, 1,
                               palette.paletteTexture, palette.paletteView, palette.paletteMemory,
                               VK_FORMAT_R8G8B8_UNORM);
}

bool FalloutAssetLoader::createVulkanTexture(const void* data, uint32_t width, uint32_t height,
                                             VkImage& image, VkImageView& imageView, VkDeviceMemory& memory,
                                             VkFormat format)
{
    VkDeviceSize imageSize = width * height * ((format == VK_FORMAT_R8G8B8_UNORM) ? 3 : 4);
    VkBuffer stagingBuffer;
    VmaAllocation stagingAlloc;
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size = imageSize;
    bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    if (vmaCreateBuffer(allocator, &bufInfo, &allocInfo, &stagingBuffer, &stagingAlloc, nullptr) != VK_SUCCESS)
        return false;
    void* mapped;
    vmaMapMemory(allocator, stagingAlloc, &mapped);
    memcpy(mapped, data, imageSize);
    vmaUnmapMemory(allocator, stagingAlloc);

    VkImageCreateInfo imgInfo{};
    imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.extent.width = width;
    imgInfo.extent.height = height;
    imgInfo.extent.depth = 1;
    imgInfo.mipLevels = 1;
    imgInfo.arrayLayers = 1;
    imgInfo.format = format;
    imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo imgAlloc{};
    imgAlloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VmaAllocation imgAllocation;
    if (vmaCreateImage(allocator, &imgInfo, &imgAlloc, &image, &imgAllocation, nullptr) != VK_SUCCESS) {
        vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);
        return false;
    }

    transitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, image, width, height);
    transitionImageLayout(image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        vmaDestroyImage(allocator, image, imgAllocation);
        vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);
        return false;
    }
    memory = imgAllocation->deviceMemory;
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);
    return true;
}

void FalloutAssetLoader::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer cmd = beginSingleTimeCommands();
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleTimeCommands(cmd);
}

void FalloutAssetLoader::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer cmd = beginSingleTimeCommands();
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    endSingleTimeCommands(cmd);
}

VkCommandBuffer FalloutAssetLoader::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = transferCommandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &allocInfo, &cmd);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    return cmd;
}

void FalloutAssetLoader::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(transferQueue);
    vkFreeCommandBuffers(device, transferCommandPool, 1, &commandBuffer);
}
