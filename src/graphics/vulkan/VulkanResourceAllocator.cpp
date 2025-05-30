#include "VulkanResourceAllocator.h"
#include <stdexcept> // For error reporting, replace with engine's logging

namespace fallout {
namespace vk {

// --- AllocatedBuffer ---
void AllocatedBuffer::Destroy(VmaAllocator allocator) {
    if (mappedData && allocation) { // Unmap if it was mapped by our allocator
        vmaUnmapMemory(allocator, allocation);
        mappedData = nullptr;
    }
    if (buffer != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }
    buffer = VK_NULL_HANDLE;
    allocation = VK_NULL_HANDLE;
    size = 0;
}

// --- AllocatedImage ---
void AllocatedImage::Destroy(VmaAllocator allocator, VkDevice device) {
    if (imageView != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    if (image != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE) {
        vmaDestroyImage(allocator, image, allocation);
    }
    imageView = VK_NULL_HANDLE;
    image = VK_NULL_HANDLE;
    allocation = VK_NULL_HANDLE;
}


// --- VulkanResourceAllocator ---
VulkanResourceAllocator::VulkanResourceAllocator() = default;

VulkanResourceAllocator::~VulkanResourceAllocator() {
    if (initialized_) {
        // Shutdown should ideally be called explicitly,
        // but this is a safeguard.
        Shutdown();
    }
}

bool VulkanResourceAllocator::Init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device) {
    if (initialized_) {
        // Log warning: Already initialized
        return true;
    }

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.instance = instance;
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;

    // VMA requires vkGetInstanceProcAddr and vkGetDeviceProcAddr if not using VMA_STATIC_VULKAN_FUNCTIONS.
    // These are usually loaded by the Vulkan loader or a library like SDL/GLFW.
    // Assuming they are available globally or through Vulkan context.
    // For this example, we'll rely on VMA finding them if linked appropriately,
    // or they could be passed in.
    // VmaVulkanFunctions vulkanFunctions = {};
    // vulkanFunctions.vkGetInstanceProcAddr = ::vkGetInstanceProcAddr; // Or from context
    // vulkanFunctions.vkGetDeviceProcAddr = ::vkGetDeviceProcAddr;     // Or from context
    // allocatorInfo.pVulkanFunctions = &vulkanFunctions;


    if (vmaCreateAllocator(&allocatorInfo, &vmaAllocator_) != VK_SUCCESS) {
        // Log error: Failed to create VMA allocator
        return false;
    }

    device_ = device;
    initialized_ = true;
    return true;
}

void VulkanResourceAllocator::Shutdown() {
    if (!initialized_) {
        return;
    }
    if (vmaAllocator_ != VK_NULL_HANDLE) {
        vmaDestroyAllocator(vmaAllocator_);
        vmaAllocator_ = VK_NULL_HANDLE;
    }
    device_ = VK_NULL_HANDLE;
    initialized_ = false;
}

bool VulkanResourceAllocator::CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo,
                                           const VmaAllocationCreateInfo& allocationCreateInfo,
                                           AllocatedBuffer& outBuffer) {
    if (!initialized_) {
        // Log error: Allocator not initialized
        return false;
    }

    VkBuffer buffer;
    VmaAllocation allocation;
    VkResult result = vmaCreateBuffer(vmaAllocator_, &bufferCreateInfo, &allocationCreateInfo,
                                      &buffer, &allocation, nullptr);
    if (result != VK_SUCCESS) {
        // Log error: Failed to create buffer with VMA
        return false;
    }

    outBuffer.buffer = buffer;
    outBuffer.allocation = allocation;
    outBuffer.size = bufferCreateInfo.size;
    outBuffer.mappedData = nullptr; // Not mapped by default by this generic function

    // If the buffer is host visible and coherent, and intended to be persistently mapped
    // we could map it here. This is often done for uniform buffers.
    // VmaAllocationInfo allocInfo;
    // vmaGetAllocationInfo(vmaAllocator_, allocation, &allocInfo);
    // if ((allocationCreateInfo.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) &&
    //     (allocInfo.memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
    //     vmaMapMemory(vmaAllocator_, allocation, &outBuffer.mappedData);
    // }

    return true;
}

bool VulkanResourceAllocator::CreateVertexBuffer(VkDeviceSize size, AllocatedBuffer& outBuffer, bool deviceLocal, bool mapped) {
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; // TRANSFER_DST for staging copy

    VmaAllocationCreateInfo allocInfo = {};
    if (deviceLocal) {
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    } else {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // For dynamic VBs updated by CPU
        if (mapped) {
            allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }
    }

    bool success = CreateBuffer(bufferInfo, allocInfo, outBuffer);
    if (success && mapped && !outBuffer.mappedData && allocInfo.usage == VMA_MEMORY_USAGE_CPU_TO_GPU) {
         // If mapped was requested for CPU_TO_GPU and CreateBuffer didn't map it (because it's generic)
        if (MapMemory(outBuffer)) {
            // Successfully mapped
        } else {
            // Log error: Failed to map vertex buffer
            outBuffer.Destroy(vmaAllocator_); // cleanup
            return false;
        }
    }
    return success;
}

bool VulkanResourceAllocator::CreateIndexBuffer(VkDeviceSize size, AllocatedBuffer& outBuffer, bool deviceLocal, bool mapped) {
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    if (deviceLocal) {
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    } else {
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
         if (mapped) {
            allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }
    }

    bool success = CreateBuffer(bufferInfo, allocInfo, outBuffer);
    if (success && mapped && !outBuffer.mappedData && allocInfo.usage == VMA_MEMORY_USAGE_CPU_TO_GPU) {
        if (MapMemory(outBuffer)) {
            // Successfully mapped
        } else {
            // Log error: Failed to map index buffer
            outBuffer.Destroy(vmaAllocator_); // cleanup
            return false;
        }
    }
    return success;
}

bool VulkanResourceAllocator::CreateUniformBuffer(VkDeviceSize size, AllocatedBuffer& outBuffer, bool persistentlyMapped) {
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // UBOs are frequently updated by CPU
    if (persistentlyMapped) {
        // VMA_ALLOCATION_CREATE_MAPPED_BIT ensures vmaCreateBuffer maps it if possible
        // and the pointer is available via VmaAllocationInfo.pMappedData or by calling vmaMapMemory
        allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    bool success = CreateBuffer(bufferInfo, allocInfo, outBuffer);
    if (success && persistentlyMapped && !outBuffer.mappedData) {
        // If CreateBuffer itself doesn't map (e.g. if MAPPED_BIT was not enough due to memory type),
        // explicitly map it.
        if (MapMemory(outBuffer)) {
           // Mapped successfully
        } else {
            // Log error: Failed to map uniform buffer. This might happen if memory is not host visible.
            // For CPU_TO_GPU, it should be host visible.
            outBuffer.Destroy(vmaAllocator_); // cleanup
            return false;
        }
    }
    return success;
}

bool VulkanResourceAllocator::CreateStagingBuffer(VkDeviceSize size, AllocatedBuffer& outBuffer, bool cpuAccessible) {
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = cpuAccessible ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    if (cpuAccessible) { // For uploading to GPU
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY; // Or CPU_TO_GPU if mapping is preferred
        allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT; // Staging buffers are usually mapped
    } else { // For reading back from GPU
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
        allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    bool success = CreateBuffer(bufferInfo, allocInfo, outBuffer);
    if (success && !outBuffer.mappedData) { // If not already mapped by VMA_ALLOCATION_CREATE_MAPPED_BIT
        if (MapMemory(outBuffer)) {
            // Mapped successfully
        } else {
             // Log error: Failed to map staging buffer
            outBuffer.Destroy(vmaAllocator_);
            return false;
        }
    }
    return success;
}


bool VulkanResourceAllocator::MapMemory(AllocatedBuffer& allocatedBuffer) {
    if (!initialized_ || allocatedBuffer.allocation == VK_NULL_HANDLE) {
        // Log error
        return false;
    }
    if (allocatedBuffer.mappedData) { // Already mapped
        return true;
    }
    VkResult result = vmaMapMemory(vmaAllocator_, allocatedBuffer.allocation, &allocatedBuffer.mappedData);
    return result == VK_SUCCESS;
}

void VulkanResourceAllocator::UnmapMemory(AllocatedBuffer& allocatedBuffer) {
    if (!initialized_ || allocatedBuffer.allocation == VK_NULL_HANDLE || !allocatedBuffer.mappedData) {
        return;
    }
    vmaUnmapMemory(vmaAllocator_, allocatedBuffer.allocation);
    allocatedBuffer.mappedData = nullptr;
}


bool VulkanResourceAllocator::CreateImage(const VkImageCreateInfo& imageCreateInfo,
                                           const VmaAllocationCreateInfo& allocationCreateInfo,
                                           AllocatedImage& outImage) {
    if (!initialized_) return false;

    VkImage image;
    VmaAllocation allocation;

    if (vmaCreateImage(vmaAllocator_, &imageCreateInfo, &allocationCreateInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
        // Log error
        return false;
    }
    outImage.image = image;
    outImage.allocation = allocation;
    // ImageView is not created here, needs to be done separately as it depends on format and usage.
    outImage.imageView = VK_NULL_HANDLE;
    return true;
}

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h" // Assumed path

// Helper function from vulkan_render.cc (or a common Vulkan utils file)
// This is a simplified version, assuming VulkanRenderer gVulkan context for device, pool, queue.
// In a real engine, these would be passed to VRA or VRA would have its own.
// For now, this is a conceptual placeholder as VRA doesn't have these directly.
// This function needs access to gVulkan.device, gVulkan.graphicsQueue and a command pool.
// This is a temporary HACK to make CreateTextureImage work without refactoring execute_single_time_commands out of vulkan_render.cc
// Ideally, VRA would have its own command pool or be passed one for transient commands.
extern VulkanRenderer gVulkan; // HACK: Accessing global gVulkan for device, queue, pool
void execute_single_time_commands_vra(std::function<void(VkCommandBuffer)> recorder) {
    if (!gVulkan.device || gVulkan.commandPools.empty() || !gVulkan.graphicsQueue) {
        // Log error: Vulkan context not available for single time command
        return;
    }
    VkCommandPool pool = gVulkan.commandPools[0]; // Use first command pool
    VkQueue queue = gVulkan.graphicsQueue;

    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = pool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(gVulkan.device, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    recorder(commandBuffer);
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(gVulkan.device, pool, 1, &commandBuffer);
}


bool VulkanResourceAllocator::CreateTextureImage(const char* filePath,
                                                 AllocatedImage& outTextureImage,
                                                 VkSampler& outTextureSampler,
                                                 VkImageLayout targetLayout) {
    if (!initialized_ || !device_) return false;

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha); // Force RGBA
    if (!pixels) {
        // Log error: Failed to load texture image using stb_image
        return false;
    }
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) * texHeight * 4; // 4 bytes per pixel for RGBA

    AllocatedBuffer stagingBuffer;
    if (!CreateStagingBuffer(imageSize, stagingBuffer, true)) {
        stbi_image_free(pixels);
        return false;
    }
    memcpy(stagingBuffer.mappedData, pixels, static_cast<size_t>(imageSize));
    stbi_image_free(pixels);

    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(texWidth);
    imageInfo.extent.height = static_cast<uint32_t>(texHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1; // TODO: Mipmap generation
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM; // Or SRGB if color space correct
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (!CreateImage(imageInfo, allocInfo, outTextureImage)) { // Uses vmaCreateImage
        stagingBuffer.Destroy(vmaAllocator_);
        return false;
    }

    // Transition image layout and copy from staging buffer
    execute_single_time_commands_vra([&](VkCommandBuffer commandBuffer) {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = outTextureImage.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0; // Tightly packed
        region.bufferImageHeight = 0; // Tightly packed
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1};
        vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, outTextureImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = targetLayout; // Usually VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    });
    stagingBuffer.Destroy(vmaAllocator_);

    // Create Image View
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = outTextureImage.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageInfo.format; // Use the same format as the image
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device_, &viewInfo, nullptr, &outTextureImage.imageView) != VK_SUCCESS) {
        // Log error
        vmaDestroyImage(vmaAllocator_, outTextureImage.image, outTextureImage.allocation); // Cleanup
        outTextureImage.image = VK_NULL_HANDLE;
        outTextureImage.allocation = VK_NULL_HANDLE;
        return false;
    }

    // Create Sampler
    VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE; // Often good to enable if supported
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(gVulkan.physicalDevice /* HACK, need physicalDevice member */, &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // TODO: Mipmap support
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f; // TODO: Mipmap support (maxLod should be num_mips -1)

    if (vkCreateSampler(device_, &samplerInfo, nullptr, &outTextureSampler) != VK_SUCCESS) {
        // Log error
        vkDestroyImageView(device_, outTextureImage.imageView, nullptr);
        vmaDestroyImage(vmaAllocator_, outTextureImage.image, outTextureImage.allocation);
        return false;
    }
    return true;
}


} // namespace vk
} // namespace fallout
