#pragma once

#include <memory>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace fallout {

struct VulkanBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocInfo{};
    VkDeviceSize size = 0;
    void* mappedData = nullptr;
};

struct VulkanImage {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
};

class VulkanMemoryManager {
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

public:
    bool init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
    void destroy();

    VmaAllocator getAllocator() const { return m_allocator; }
    VkDevice getDevice() const { return m_device; }

    VulkanBuffer createVertexBuffer(const void* data, VkDeviceSize size);
    VulkanBuffer createUniformBuffer(VkDeviceSize size);
    VulkanBuffer createStagingBuffer(VkDeviceSize size);

    VulkanImage createSpriteAtlas(uint32_t width, uint32_t height, const void* data);
    VulkanImage createPaletteTexture(const uint8_t* paletteData, uint32_t paletteCount);

    void destroyBuffer(VulkanBuffer& buffer);
    void destroyImage(VulkanImage& image);

private:
    void uploadDataToBuffer(VulkanBuffer& buffer, const void* data, VkDeviceSize size);
    void uploadDataToImage(VulkanImage& image, const void* data, size_t sizeBytes);
};

class ManagedBuffer {
    VulkanBuffer m_buffer{};
    VmaAllocator m_allocator = VK_NULL_HANDLE;

public:
    ManagedBuffer(VmaAllocator alloc, const VulkanBuffer& buf) : m_buffer(buf), m_allocator(alloc) {}
    ~ManagedBuffer();

    VulkanBuffer* operator->() { return &m_buffer; }
    const VulkanBuffer* operator->() const { return &m_buffer; }
};

class ManagedImage {
    VulkanImage m_image{};
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

public:
    ManagedImage(VmaAllocator alloc, VkDevice dev, const VulkanImage& img)
        : m_image(img)
        , m_allocator(alloc)
        , m_device(dev)
    {
    }
    ~ManagedImage();

    VulkanImage* operator->() { return &m_image; }
    const VulkanImage* operator->() const { return &m_image; }
};

using BufferPtr = std::unique_ptr<ManagedBuffer>;
using ImagePtr = std::unique_ptr<ManagedImage>;

} // namespace fallout
