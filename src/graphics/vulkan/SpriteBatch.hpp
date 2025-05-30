#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/vec2.hpp>
#include <vector>

struct Sprite {
    glm::vec2 pos;
    glm::vec2 size;
    glm::vec2 uv0;
    glm::vec2 uv1;
    uint32_t paletteIndex;
};

class SpriteBatch {
public:
    SpriteBatch();
    ~SpriteBatch();

    bool init(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t queueFamily);
    void destroy();

    void draw(const Sprite& s);
    void flush(VkCommandBuffer cb);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_vertexAlloc = nullptr;
    VkBuffer m_instanceBuffer = VK_NULL_HANDLE;
    VmaAllocation m_instanceAlloc = nullptr;
    std::vector<Sprite> m_sprites;
};
