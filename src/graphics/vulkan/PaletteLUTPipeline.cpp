#include "PaletteLUTPipeline.h"
#include <vector>
#include <array>

using namespace fallout::vk;

VkResult PaletteLUTPipeline::create(VkDevice device, VkExtent2D extent, VkRenderPass renderPass)
{
    VkDescriptorSetLayoutBinding bindings[2]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    setInfo.bindingCount = 2;
    setInfo.pBindings = bindings;
    if(vkCreateDescriptorSetLayout(device,&setInfo,nullptr,&setLayout)!=VK_SUCCESS)
        return VK_ERROR_INITIALIZATION_FAILED;

    VkPushConstantRange pc{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(float)};
    VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &setLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pc;
    if(vkCreatePipelineLayout(device,&layoutInfo,nullptr,&layout)!=VK_SUCCESS)
        return VK_ERROR_INITIALIZATION_FAILED;

    // Shader modules
    extern std::vector<char> readFile(const char*); // reuse helper
    VkShaderModule vertModule=VK_NULL_HANDLE, fragModule=VK_NULL_HANDLE;
    {
        auto code = readFile("shaders/sprite.vert.spv");
        VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        sm.codeSize = code.size();
        sm.pCode = reinterpret_cast<const uint32_t*>(code.data());
        vkCreateShaderModule(device,&sm,nullptr,&vertModule);
    }
    {
        auto code = readFile("shaders/palette_lut.frag.spv");
        VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        sm.codeSize = code.size();
        sm.pCode = reinterpret_cast<const uint32_t*>(code.data());
        vkCreateShaderModule(device,&sm,nullptr,&fragModule);
    }
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkViewport vp{0,0,(float)extent.width,(float)extent.height,0,1};
    VkRect2D sc{{0,0},extent};
    VkPipelineViewportStateCreateInfo vpstate{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vpstate.viewportCount=1;vpstate.pViewports=&vp;vpstate.scissorCount=1;vpstate.pScissors=&sc;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.lineWidth=1;rs.cullMode=VK_CULL_MODE_NONE;rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState att{};att.colorWriteMask=0xF;att.blendEnable=VK_FALSE;
    VkPipelineColorBlendStateCreateInfo cb{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount=1;cb.pAttachments=&att;
    VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gp.stageCount=2;gp.pStages=stages;gp.pVertexInputState=&vi;gp.pInputAssemblyState=&ia;gp.pViewportState=&vpstate;gp.pRasterizationState=&rs;gp.pMultisampleState=&ms;gp.pColorBlendState=&cb;gp.layout=layout;gp.renderPass=renderPass;
    VkResult res=vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&gp,nullptr,&pipeline);
    vkDestroyShaderModule(device,vertModule,nullptr);
    vkDestroyShaderModule(device,fragModule,nullptr);
    return res;
}

void PaletteLUTPipeline::destroy(VkDevice device)
{
    if(pipeline) vkDestroyPipeline(device,pipeline,nullptr);
    if(layout) vkDestroyPipelineLayout(device,layout,nullptr);
    if(setLayout) vkDestroyDescriptorSetLayout(device,setLayout,nullptr);
    pipeline=VK_NULL_HANDLE;layout=VK_NULL_HANDLE;setLayout=VK_NULL_HANDLE;
}
