#pragma once

#include <vulkan/vulkan.h>

namespace fallout {
namespace vk {

    class VulkanSpritePipeline {
    public:
        bool createGraphicsPipeline(VkDevice device, VkExtent2D swapchainExtent, VkRenderPass renderPass);
        void destroy(VkDevice device);

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
    };

} // namespace vk
} // namespace fallout
