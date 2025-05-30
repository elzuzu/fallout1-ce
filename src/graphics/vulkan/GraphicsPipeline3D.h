#ifndef FALLOUT_GRAPHICS_VULKAN_GRAPHICS_PIPELINE_3D_H_
#define FALLOUT_GRAPHICS_VULKAN_GRAPHICS_PIPELINE_3D_H_

#include <vulkan/vulkan.h>

namespace fallout {
namespace vk {

class GraphicsPipeline3D {
public:
    GraphicsPipeline3D(VkDevice device);
    ~GraphicsPipeline3D();

    bool Create(VkFormat colorImageFormat, VkPipelineCache pipelineCache);
    void Destroy();

    VkPipelineLayout GetPipelineLayout() const { return pipelineLayout_; }
    VkPipeline GetPipeline() const { return pipeline_; }
    // Combined layout for UBO and Texture Sampler
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptorSetLayout_; }

private:
    bool CreateShaderModules();
    void DestroyShaderModules();
    bool CreateDescriptorSetLayout(); // For UBO (binding 0) and Texture Sampler (binding 1)

    VkDevice device_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE; // Combined layout
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;

    VkShaderModule vertShaderModule_ = VK_NULL_HANDLE;
    VkShaderModule fragShaderModule_ = VK_NULL_HANDLE;
};
// TODO: Consider moving placeholder shaders to separate .spv files and loading them.

} // namespace vk
} // namespace fallout

#endif // FALLOUT_GRAPHICS_VULKAN_GRAPHICS_PIPELINE_3D_H_
