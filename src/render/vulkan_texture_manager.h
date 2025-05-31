#ifndef FALLOUT_RENDER_VULKAN_TEXTURE_MANAGER_H_
#define FALLOUT_RENDER_VULKAN_TEXTURE_MANAGER_H_

#include <unordered_map>
#include <string>
#include <mutex>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "render/fallout_memory_manager.h"

namespace fallout {


class VulkanTextureManager {
public:
    struct Texture {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VmaAllocation memory = VK_NULL_HANDLE;
        VkExtent2D extent{};
        uint32_t mipLevels = 1;
    };

    VulkanTextureManager() = default;

    bool init(VkPhysicalDevice physicalDevice, VkDevice device,
        FalloutMemoryManager* memoryManager, VkCommandPool commandPool, VkQueue queue);
    void destroy();

    void loadTextureAsync(const std::string& path);
    const Texture* getTexture(const std::string& path) const;

private:
    bool loadTexture(const std::string& path, Texture& out);
    void generateMipmaps(Texture& texture, VkFormat format,
        int32_t texWidth, int32_t texHeight);

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    FalloutMemoryManager* m_memoryManager = nullptr;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;

    mutable std::mutex m_cacheMutex;
    std::unordered_map<std::string, Texture> textureCache;
};

} // namespace fallout

#endif /* FALLOUT_RENDER_VULKAN_TEXTURE_MANAGER_H_ */
