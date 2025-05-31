#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace fallout {

struct VulkanBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocInfo{};
    VkDeviceSize size = 0;
    void* mappedData = nullptr;
    std::string debugName;
};

struct VulkanImage {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
    std::string debugName;
};

class VulkanMemoryManager {
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

    std::unordered_map<VkBuffer, std::string> m_bufferNames;
    std::unordered_map<VkImage, std::string> m_imageNames;
    size_t m_totalAllocatedBytes = 0;

public:
    bool init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
    void destroy();
    void printMemoryStats();

    VmaAllocator getAllocator() const { return m_allocator; }
    VkDevice getDevice() const { return m_device; }

    VulkanBuffer createVertexBuffer(const void* data, VkDeviceSize size, const std::string& name = "");
    VulkanBuffer createIndexBuffer(const void* data, VkDeviceSize size, const std::string& name = "");
    VulkanBuffer createUniformBuffer(VkDeviceSize size, bool hostVisible = true, const std::string& name = "");
    VulkanBuffer createStagingBuffer(VkDeviceSize size, const std::string& name = "");

    VulkanImage createSpriteAtlas(uint32_t width, uint32_t height, const void* data = nullptr, const std::string& name = "");
    VulkanImage createPaletteTexture(const uint8_t* paletteData, uint32_t paletteCount, const std::string& name = "");

    void destroyBuffer(VulkanBuffer& buffer);
    void destroyImage(VulkanImage& image);

private:
    void uploadDataToBuffer(VulkanBuffer& buffer, const void* data, VkDeviceSize size);
    void uploadDataToImage(VulkanImage& image, const void* data, size_t sizeBytes);
};

template<typename T>
class VulkanResource {
protected:
    T resource{};
    VmaAllocator allocator = VK_NULL_HANDLE;
    bool valid = false;

public:
    VulkanResource() = default;
    VulkanResource(const VulkanResource&) = delete;
    VulkanResource& operator=(const VulkanResource&) = delete;

    VulkanResource(VulkanResource&& other) noexcept
        : resource(other.resource)
        , allocator(other.allocator)
        , valid(other.valid)
    {
        other.valid = false;
    }

    VulkanResource& operator=(VulkanResource&& other) noexcept
    {
        if (this != &other) {
            cleanup();
            resource = other.resource;
            allocator = other.allocator;
            valid = other.valid;
            other.valid = false;
        }
        return *this;
    }

    virtual ~VulkanResource() { cleanup(); }

    const T& get() const { return resource; }
    T& get() { return resource; }

    operator bool() const { return valid; }

protected:
    virtual void cleanup() = 0;
};

class ManagedBuffer : public VulkanResource<VulkanBuffer> {
public:
    ManagedBuffer(VmaAllocator alloc, const VulkanBuffer& buf)
    {
        allocator = alloc;
        resource = buf;
        valid = (resource.buffer != VK_NULL_HANDLE);
    }

protected:
    void cleanup() override
    {
        if (valid && resource.buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(allocator, resource.buffer, resource.allocation);
            resource = {};
            valid = false;
        }
    }
};

class ManagedImage : public VulkanResource<VulkanImage> {
    VkDevice device = VK_NULL_HANDLE;

public:
    ManagedImage(VmaAllocator alloc, VkDevice dev, const VulkanImage& img)
    {
        allocator = alloc;
        device = dev;
        resource = img;
        valid = (resource.image != VK_NULL_HANDLE);
    }

protected:
    void cleanup() override
    {
        if (valid && resource.image != VK_NULL_HANDLE) {
            if (resource.imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device, resource.imageView, nullptr);
            }
            vmaDestroyImage(allocator, resource.image, resource.allocation);
            resource = {};
            valid = false;
        }
    }
};

using BufferPtr = std::unique_ptr<ManagedBuffer>;
using ImagePtr = std::unique_ptr<ManagedImage>;

} // namespace fallout
