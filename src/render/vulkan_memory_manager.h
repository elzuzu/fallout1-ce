#ifndef FALLOUT_RENDER_VULKAN_MEMORY_MANAGER_H_
#define FALLOUT_RENDER_VULKAN_MEMORY_MANAGER_H_

#include <vector>
#include <vulkan/vulkan.h>

namespace fallout {

class VulkanMemoryManager {
public:
    VulkanMemoryManager() = default;

    bool init(VkPhysicalDevice physicalDevice, VkDevice device);
    void destroy();

    enum class PoolType {
        Vertex,
        Index,
        Uniform,
    };

    struct Allocation {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize offset = 0;
    };

    // Allocate a buffer from the given pool. Returns false on failure.
    bool allocate_buffer(PoolType type, VkDeviceSize size, VkBufferUsageFlags usage, Allocation& out);

    // Create a texture image using a temporary staging buffer.
    bool create_texture_with_staging(const void* pixels, VkDeviceSize size,
        const VkImageCreateInfo& imageInfo, VkCommandPool commandPool, VkQueue queue,
        VkImage& image, VkDeviceMemory& memory);

private:
    struct MemoryPool {
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
        VkDeviceSize offset = 0;
    };

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    MemoryPool m_vertexPool {};
    MemoryPool m_indexPool {};
    MemoryPool m_uniformPool {};

    uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    bool init_pool(MemoryPool& pool, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    bool create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& memory);
};

} // namespace fallout

#endif /* FALLOUT_RENDER_VULKAN_MEMORY_MANAGER_H_ */
