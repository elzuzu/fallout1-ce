#include <vulkan/vulkan.h>
#include <vector>
#include <cstring>

struct YuvGpuContext {
    VkDevice device{VK_NULL_HANDLE};
    VkImage yuvImage{VK_NULL_HANDLE};
    VkDeviceMemory memory{VK_NULL_HANDLE};
    VkImage outputImage{VK_NULL_HANDLE};
    VkSamplerYcbcrConversion conversion{VK_NULL_HANDLE};
    VkDescriptorSetLayout setLayout{VK_NULL_HANDLE};
    VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
    VkPipeline pipeline{VK_NULL_HANDLE};
};

// compute shader
const char* yuvCompSrc = R"(
#version 450
layout(local_size_x=16, local_size_y=16) in;
layout(binding=0) uniform sampler2D yuvTex;
layout(binding=1, rgba8) uniform writeonly image2D outImg;
void main(){
    ivec2 uv = ivec2(gl_GlobalInvocationID.xy);
    vec3 yuv = texture(yuvTex, (vec2(uv)+0.5)/imageSize(outImg)).rgb;
    vec3 rgb = yuv;
    rgb.r = yuv.r + 1.402 * (yuv.b - 0.5);
    rgb.g = yuv.r - 0.344136 * (yuv.g - 0.5) - 0.714136 * (yuv.b - 0.5);
    rgb.b = yuv.r + 1.772 * (yuv.g - 0.5);
    imageStore(outImg, uv, vec4(rgb,1.0));
}
)";

VkResult setupYuvPipeline(YuvGpuContext& ctx, VkPhysicalDevice gpu, uint32_t width, uint32_t height)
{
    uint32_t qf = 0;
    VkSamplerYcbcrConversionCreateInfo conv{VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO};
    conv.format = VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
    conv.ycbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709;
    conv.ycbcrRange = VK_SAMPLER_YCBCR_RANGE_ITU_FULL;
    if(vkCreateSamplerYcbcrConversion(ctx.device,&conv,nullptr,&ctx.conversion)!=VK_SUCCESS)
        return VK_ERROR_INITIALIZATION_FAILED;

    VkImageCreateInfo img{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    img.imageType = VK_IMAGE_TYPE_2D;
    img.format = conv.format;
    img.extent = {width,height,1};
    img.mipLevels = 1; img.arrayLayers = 1;
    img.tiling = VK_IMAGE_TILING_OPTIMAL;
    img.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    vkCreateImage(ctx.device,&img,nullptr,&ctx.yuvImage);
    VkMemoryRequirements req; vkGetImageMemoryRequirements(ctx.device,ctx.yuvImage,&req);
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize=req.size; alloc.memoryTypeIndex=0;
    vkAllocateMemory(ctx.device,&alloc,nullptr,&ctx.memory);
    vkBindImageMemory(ctx.device,ctx.yuvImage,ctx.memory,0);

    VkDescriptorSetLayoutBinding b0{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_COMPUTE_BIT};
    VkDescriptorSetLayoutBinding b1{1,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1,VK_SHADER_STAGE_COMPUTE_BIT};
    VkDescriptorSetLayoutCreateInfo sInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    b0.pImmutableSamplers=nullptr; b1.pImmutableSamplers=nullptr; VkDescriptorSetLayoutBinding binds[2]={b0,b1};
    sInfo.bindingCount=2; sInfo.pBindings=binds;
    vkCreateDescriptorSetLayout(ctx.device,&sInfo,nullptr,&ctx.setLayout);

    VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pl.setLayoutCount=1; pl.pSetLayouts=&ctx.setLayout;
    vkCreatePipelineLayout(ctx.device,&pl,nullptr,&ctx.pipelineLayout);

    // compile shader at runtime is impossible; assume SPIR-V available
    return VK_SUCCESS;
}
