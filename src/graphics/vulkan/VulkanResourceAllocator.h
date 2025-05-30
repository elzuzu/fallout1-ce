#ifndef FALLOUT_GRAPHICS_VULKAN_RESOURCE_ALLOCATOR_H_
#define FALLOUT_GRAPHICS_VULKAN_RESOURCE_ALLOCATOR_H_

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace fallout {
namespace vk {

// Wrapper for a Vulkan buffer and its VMA allocation
struct AllocatedBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    void* mappedData = nullptr; // If persistently mapped

    // Basic cleanup helper
    void Destroy(VmaAllocator allocator);
};

// Wrapper for a Vulkan image and its VMA allocation
struct AllocatedImage {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE; // Optional, can be created separately

    // Basic cleanup helper
    void Destroy(VmaAllocator allocator, VkDevice device);
};


class VulkanResourceAllocator {
public:
    VulkanResourceAllocator();
    ~VulkanResourceAllocator();

    // Initialization and shutdown
    bool Init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
    void Shutdown();

    VmaAllocator GetVmaAllocator() const { return vmaAllocator_; }

    // Buffer creation methods
    // For vertex/index buffers, typically GPU_ONLY. Staging buffer needed for initial data.
    bool CreateVertexBuffer(VkDeviceSize size, AllocatedBuffer& outBuffer, bool deviceLocal = true, bool mapped = false);
    bool CreateIndexBuffer(VkDeviceSize size, AllocatedBuffer& outBuffer, bool deviceLocal = true, bool mapped = false);
    // Uniform buffers are typically CPU_TO_GPU and often persistently mapped
    bool CreateUniformBuffer(VkDeviceSize size, AllocatedBuffer& outBuffer, bool persistentlyMapped = true);
    // Staging buffers are CPU_ONLY for uploading, or GPU_ONLY for readback
    bool CreateStagingBuffer(VkDeviceSize size, AllocatedBuffer& outBuffer, bool cpuAccessible = true);

    // Generic buffer creation (can be used for other types or more control)
    bool CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo,
                      const VmaAllocationCreateInfo& allocationCreateInfo,
                      AllocatedBuffer& outBuffer);

    // Image creation (basic example)
    bool CreateImage(const VkImageCreateInfo& imageCreateInfo,
                     const VmaAllocationCreateInfo& allocationCreateInfo,
                     AllocatedImage& outImage); // Basic image creation using VMA

    // Texture specific creation
    bool CreateTextureImage(const char* filePath,
                            AllocatedImage& outTextureImage,
                            VkSampler& outTextureSampler,
                            VkImageLayout initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    // Memory mapping
    bool MapMemory(AllocatedBuffer& allocatedBuffer);
    void UnmapMemory(AllocatedBuffer& allocatedBuffer);

    // Utility to copy data to a device-local buffer using a staging buffer
    // This is a higher-level function that uses the allocator.
    // bool UploadDataToBuffer(VkCommandBuffer cmdBuffer, const void* data, VkDeviceSize size, AllocatedBuffer& targetDeviceBuffer);

private:
    VmaAllocator vmaAllocator_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE; // Store for destroying views, etc.
    bool initialized_ = false;
    // Memory pools for specific buffer types
    VmaPool vertexBufferPool_ = VK_NULL_HANDLE;
    VmaPool indexBufferPool_ = VK_NULL_HANDLE;
    VmaPool uniformBufferPool_ = VK_NULL_HANDLE;
};

} // namespace vk
} // namespace fallout

#endif // FALLOUT_GRAPHICS_VULKAN_RESOURCE_ALLOCATOR_H_
