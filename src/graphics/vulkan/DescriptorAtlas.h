#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace fallout {
class DescriptorAtlas {
public:
    bool init(VkDevice device, uint32_t maxAtlases);
    void update(VkDevice device, uint32_t index, const VkDescriptorImageInfo& info);
    VkDescriptorSetLayout layout() const { return m_layout; }
    VkDescriptorSet set() const { return m_set; }
private:
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
    VkDescriptorSet m_set = VK_NULL_HANDLE;
};
}
