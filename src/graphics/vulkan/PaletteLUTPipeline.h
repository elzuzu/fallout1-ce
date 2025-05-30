#pragma once
#include <vulkan/vulkan.h>
namespace fallout {
namespace vk {
struct PaletteLUTPipeline {
    VkPipeline pipeline{VK_NULL_HANDLE};
    VkPipelineLayout layout{VK_NULL_HANDLE};
    VkDescriptorSetLayout setLayout{VK_NULL_HANDLE};
    VkDescriptorPool pool{VK_NULL_HANDLE};
    VkDescriptorSet set{VK_NULL_HANDLE};
    VkImageView lutView{VK_NULL_HANDLE};
    VkSampler lutSampler{VK_NULL_HANDLE};
    VkImageView indexView{VK_NULL_HANDLE};
    VkSampler indexSampler{VK_NULL_HANDLE};
    VkImage lutImage{VK_NULL_HANDLE};
    VkDeviceMemory lutMemory{VK_NULL_HANDLE};
    VkResult create(VkDevice device, VkExtent2D extent, VkRenderPass renderPass);
    void destroy(VkDevice device);
};
}
}
