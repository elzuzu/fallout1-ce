#include "graphics/vulkan/GraphicsPipeline3D.h"

#include <vector>
#include <array>

// Placeholder SPIR-V code (minimal valid shaders)
// These are conceptual, minimal SPIR-V hex codes.
// Real versions would be compiled from GLSL.
#include "graphics/RenderableTypes.h" // For graphics::Vertex for sizeof and offsetof

// Vertex Shader: UBO, Pos, Normal, UV, Color inputs. Outputs UV, Color.
const uint32_t default_vert_spv[] = {
    0x07230203,0x00010300,0x000d000d,0x00000030,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x000a000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x0000000c,0x00000010,0x00000016,
    0x0000001d,0x00000020,0x00000026,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,
    0x6e69616d,0x00000000,0x00050005,0x0000000c,0x506e6973,0x696f6974,0x0000006e,0x00050005,
    0x00000010,0x4e6e696f,0x6c616d72,0x00000000,0x00040005,0x00000013,0x56556e69,0x00000000,
    0x00050005,0x00000016,0x436e696f,0x726f6c6f,0x00000000,0x00040005,0x0000001a,0x424f5520,
    0x00000000,0x00060005,0x0000001d,0x6f62752e,0x6c65646f,0x0000006d,0x00060005,0x0000001e,
    0x752e6f62,0x77656976,0x00000000,0x00060005,0x0000001f,0x2e6f6275,0x6a6f7270,0x00000000,
    0x00050005,0x00000020,0x5674756f,0x00005655,0x00050005,0x00000026,0x4374756f,0x726f6c6f,
    0x00000000,0x00030005,0x00000028,0x00000000,0x00040047,0x0000000c,0x0000001e,0x00000000,
    0x00040047,0x00000010,0x0000001e,0x00000001,0x00040047,0x00000013,0x0000001e,0x00000002,
    0x00040047,0x00000016,0x0000001e,0x00000003,0x00040047,0x0000001a,0x00000007,0x00000000,
    0x00040047,0x0000001d,0x00000022,0x00000000,0x00040047,0x0000001d,0x00000021,0x00000000,
    0x00040047,0x00000020,0x0000001e,0x00000000,0x00040047,0x00000026,0x0000001e,0x00000001,
    0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,
    0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,0x00000008,0x00000006,0x00000003,
    0x00040017,0x00000009,0x00000006,0x00000002,0x00040017,0x0000000a,0x00000006,0x00000004,
    0x00040020,0x0000000b,0x00000003,0x00000007,0x0004003b,0x0000000b,0x0000000c,0x00000003,
    0x00040020,0x0000000d,0x00000003,0x00000008,0x0004003b,0x0000000d,0x00000010,0x00000003,
    0x00040020,0x00000011,0x00000001,0x00000009,0x0004003b,0x00000011,0x00000013,0x00000001,
    0x00040020,0x00000014,0x00000003,0x0000000a,0x0004003b,0x00000014,0x00000016,0x00000003,
    0x0004001e,0x0000001a,0x00000006,0x0000000a,0x0000000a,0x00050041,0x0000001d,0x0000001b,
    0x0000001a,0x00000000,0x0004003b,0x0000001b,0x0000001c,0x00000000,0x0004003b,0x0000001b,
    0x0000001e,0x00000001,0x0004003b,0x0000001b,0x0000001f,0x00000002,0x0006002c,0x0000001a,
    0x00000007,0x0000001c,0x0000001e,0x0000001f,0x0000001f,0x0003003e,0x00000013,0x00000011,
    0x0003003e,0x00000016,0x00000014,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,
    0x000200f8,0x00000005,0x0004003d,0x00000007,0x0000000e,0x0000000c,0x00050051,0x00000006,
    0x0000000f,0x0000000e,0x00000000,0x00050051,0x00000006,0x00000012,0x0000000e,0x00000001,
    0x0004003d,0x00000009,0x00000015,0x00000013,0x0004003d,0x0000000a,0x00000017,0x00000016,
    0x00050041,0x0000000f,0x00000018,0x00000012,0x00000000,0x0004003d,0x00000006,0x00000019,
    0x00000018,0x00050041,0x0000001d,0x0000001b,0x0000001a,0x00000000,0x00060050,0x00000006,
    0x0000001c,0x00000019,0x0000001b,0x00000000,0x00050041,0x0000001e,0x0000001b,0x0000001a,
    0x00000001,0x00060050,0x00000006,0x00000021,0x00000019,0x0000001e,0x0000001b,0x00050041,
    0x0000001f,0x0000001b,0x0000001a,0x00000002,0x00060050,0x00000006,0x00000022,0x00000019,
    0x0000001f,0x0000001b,0x00050041,0x00000006,0x00000023,0x0000000e,0x00000004,0x00060050,
    0x00000006,0x00000024,0x00000023,0x00000021,0x00000022,0x00060050,0x00000006,0x00000025,
    0x00000023,0x00000024,0x0000001c,0x0003003e,0x0000000c,0x00000025,0x0003003e,0x00000013,
    0x00000015,0x0003003e,0x00000016,0x00000017,0x0003003e,0x00000020,0x00000015,0x0003003e,
    0x00000026,0x00000017,0x000100fd,0x00010038
};
// Fragment Shader: Takes UV, Color. Samples texture, multiplies by color.
const uint32_t default_frag_spv[] = {
    0x07230203,0x00010300,0x000d000a,0x00000013,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000c,0x0000000f,
    0x00040005,0x00000004,0x6e69616d,0x00000000,0x00050005,0x00000009,0x56557475,0x0000006f,
    0x00050005,0x0000000c,0x4374756f,0x726f6c6f,0x00000000,0x00040005,0x0000000f,0x6f6c6f4374,
    0x0072756f,0x00050005,0x00000011,0x53786574,0x6c706d61,0x00000072,0x00040047,0x00000009,
    0x0000001e,0x00000000,0x00040047,0x0000000c,0x0000001e,0x00000001,0x00040047,0x0000000f,
    0x0000001e,0x00000000,0x00040047,0x00000011,0x00000022,0x00000000,0x00040047,0x00000011,
    0x00000021,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,
    0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000002,0x00040017,0x00000008,
    0x00000006,0x00000004,0x00040020,0x0000000a,0x00000003,0x00000007,0x0004003b,0x0000000a,
    0x00000009,0x00000003,0x00040020,0x0000000b,0x00000001,0x00000008,0x0004003b,0x0000000b,
    0x0000000c,0x00000001,0x00040020,0x0000000d,0x00000003,0x00000008,0x00040020,0x0000000e,
    0x00000001,0x00000008,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x0004001c,0x00000010,
    0x00000006,0x00000008,0x00000007,0x00040020,0x00000012,0x00000001,0x00000010,0x00050036,
    0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003d,0x00000010,
    0x00000010,0x00000011,0x00050041,0x00000012,0x00000006,0x00000010,0x00000000,0x0004003d,
    0x00000007,0x0000000a,0x00000009,0x00050057,0x00000008,0x0000000d,0x00000006,0x0000000a,
    0x0003003e,0x0000000f,0x0000000d,0x000100fd,0x00010038
};

// Placeholder for graphics::Vertex, assuming it's defined in RenderableTypes.h
// struct Vertex { Vec3 pos; Vec3 normal; Vec2 uv; Vec4 color; };
// Make sure RenderableTypes.h is included if this is used.
#include "graphics/RenderableTypes.h"


namespace fallout {
namespace vk {

GraphicsPipeline3D::GraphicsPipeline3D(VkDevice device) : device_(device) {}

GraphicsPipeline3D::~GraphicsPipeline3D() {
    // Destroy() should be called explicitly before destruction
}

bool GraphicsPipeline3D::CreateShaderModules() {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = sizeof(default_vert_spv);
    createInfo.pCode = default_vert_spv;

    if (vkCreateShaderModule(device_, &createInfo, nullptr, &vertShaderModule_) != VK_SUCCESS) {
        return false;
    }

    createInfo.codeSize = sizeof(default_frag_spv);
    createInfo.pCode = default_frag_spv;
    if (vkCreateShaderModule(device_, &createInfo, nullptr, &fragShaderModule_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, vertShaderModule_, nullptr);
        vertShaderModule_ = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

void GraphicsPipeline3D::DestroyShaderModules() {
    if (vertShaderModule_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device_, vertShaderModule_, nullptr);
        vertShaderModule_ = VK_NULL_HANDLE;
    }
    if (fragShaderModule_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device_, fragShaderModule_, nullptr);
        fragShaderModule_ = VK_NULL_HANDLE;
    }
}

bool GraphicsPipeline3D::Create(VkFormat colorImageFormat, VkPipelineCache pipelineCache) {
    if (!CreateShaderModules()) {
        // TODO: Add logging: "Failed to create shader modules for 3D pipeline"
        return false;
    }

    // Pipeline Layout (empty for now)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        // TODO: Add logging: "Failed to create 3D pipeline layout"
        DestroyShaderModules();
        return false;
    }

    // --- Start of Graphics Pipeline Create Info ---
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    // Shader stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule_;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule_;
    fragShaderStageInfo.pName = "main";

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();

    // Vertex input (empty for now, assuming vertex data is hardcoded in shader or comes from elsewhere)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    pipelineInfo.pInputAssemblyState = &inputAssembly;

    // Viewport and Scissor (will be dynamic)
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    // viewportState.pViewports = nullptr; // Dynamic
    viewportState.scissorCount = 1;
    // viewportState.pScissors = nullptr; // Dynamic
    pipelineInfo.pViewportState = &viewportState;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // Or counter-clockwise depending on model conventions
    rasterizer.depthBiasEnable = VK_FALSE;
    pipelineInfo.pRasterizationState = &rasterizer;

    // Multisampling (disabled)
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineInfo.pMultisampleState = &multisampling;

    // Depth and Stencil testing (disabled for now)
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE; // Enable for 3D
    depthStencil.depthWriteEnable = VK_FALSE; // Enable for 3D
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // Common for 3D
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    pipelineInfo.pDepthStencilState = &depthStencil; // Set to nullptr if not used

    // Color blending (alpha blending for the first attachment)
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    pipelineInfo.pColorBlendState = &colorBlending;

    // Dynamic states
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    pipelineInfo.pDynamicState = &dynamicState;

    // Render pass (Dynamic Rendering)
    pipelineInfo.renderPass = VK_NULL_HANDLE; // Set to NULL for dynamic rendering
    pipelineInfo.subpass = 0; // Not used with dynamic rendering if renderPass is NULL

    // Dynamic Rendering Info
    VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorImageFormat;
    // TODO: Set depthAttachmentFormat and stencilAttachmentFormat if depth/stencil testing is used
    pipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    pipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    pipelineInfo.pNext = &pipelineRenderingCreateInfo; // Chain this info

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.layout = pipelineLayout_;

    if (vkCreateGraphicsPipelines(device_, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline_) != VK_SUCCESS) {
        // TODO: Add logging: "Failed to create 3D graphics pipeline"
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
        DestroyShaderModules();
        return false;
    }

    // Shaders can be destroyed after pipeline creation
    DestroyShaderModules(); // Destroy original ones if they existed
    return true;
}

bool GraphicsPipeline3D::CreateDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
    // UBO Binding
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    // Texture Sampler Binding
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS) {
        // TODO: Logging
        return false;
    }
    return true;
}

bool GraphicsPipeline3D::Create(VkFormat colorImageFormat, VkPipelineCache pipelineCache) {
    // bool depthTestEnable = true; // Added to make it a parameter if needed, or use a member.
    // For now, assume it's always true for this 3D pipeline. Hardcoded true below.
    if (!CreateShaderModules()) {
        // TODO: Add logging: "Failed to create shader modules for 3D pipeline"
        return false;
    }

    if (!CreateDescriptorSetLayout()) {
        // TODO: Add logging
        DestroyShaderModules();
        return false;
    }

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; // One descriptor set for UBO
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        // TODO: Add logging: "Failed to create 3D pipeline layout"
        vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
        descriptorSetLayout_ = VK_NULL_HANDLE;
        DestroyShaderModules();
        return false;
    }

    // --- Start of Graphics Pipeline Create Info ---
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    // Shader stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule_;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule_;
    fragShaderStageInfo.pName = "main";

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();

    // Vertex input
    // Define binding description
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(graphics::Vertex); // Using the new Vertex struct
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Define attribute descriptions
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
    // Position (location 0)
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // Vec3
    attributeDescriptions[0].offset = offsetof(graphics::Vertex, position);
    // Normal (location 1) - used by new conceptual shader, though not for lighting yet
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // Vec3
    attributeDescriptions[1].offset = offsetof(graphics::Vertex, normal);
    // UV (location 2)
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;    // Vec2
    attributeDescriptions[2].offset = offsetof(graphics::Vertex, uv);
    // Color (location 3)
    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT; // Vec4
    attributeDescriptions[3].offset = offsetof(graphics::Vertex, color);


    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    pipelineInfo.pInputAssemblyState = &inputAssembly;

    // Viewport and Scissor (will be dynamic)
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    pipelineInfo.pViewportState = &viewportState;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Standard for glTF
    rasterizer.depthBiasEnable = VK_FALSE;
    pipelineInfo.pRasterizationState = &rasterizer;

    // Multisampling (disabled)
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineInfo.pMultisampleState = &multisampling;

    // Depth and Stencil testing
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE; // Enable for 3D
    depthStencil.depthWriteEnable = VK_TRUE; // Enable for 3D
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    pipelineInfo.pDepthStencilState = &depthStencil;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE; // Typically false for opaque 3D, true for transparent
    // ... (set blend factors if blendEnable is true)
    // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;


    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    pipelineInfo.pColorBlendState = &colorBlending;

    // Dynamic states
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    pipelineInfo.pDynamicState = &dynamicState;

    // Render pass (Dynamic Rendering)
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorImageFormat;
    // IMPORTANT: Need a depth attachment format if depth test is enabled
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT; // This should ideally be queried or passed in.
                                                 // Must match the depth image format used by VulkanRenderer.
    pipelineRenderingCreateInfo.depthAttachmentFormat = VK_TRUE ? depthFormat : VK_FORMAT_UNDEFINED; // depthTestEnable is effectively true
    pipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
    pipelineInfo.pNext = &pipelineRenderingCreateInfo;

    pipelineInfo.layout = pipelineLayout_; // This is the pipelineLayout created earlier with the combined descriptorSetLayout_

    if (vkCreateGraphicsPipelines(device_, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline_) != VK_SUCCESS) {
        // TODO: Add logging
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
        descriptorSetLayout_ = VK_NULL_HANDLE;
        DestroyShaderModules();
        return false;
    }

    DestroyShaderModules(); // Shaders can be destroyed after pipeline creation
    return true;
}

void GraphicsPipeline3D::Destroy() {
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
        descriptorSetLayout_ = VK_NULL_HANDLE;
    }
    DestroyShaderModules();
}

} // namespace vk
} // namespace fallout
