#include "graphics/vulkan/DescriptorAtlas.h"
#include <array>

namespace fallout {

bool DescriptorAtlas::init(VkDevice device, uint32_t maxAtlases)
{
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = maxAtlases;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.pImmutableSamplers = nullptr;
    VkDescriptorBindingFlags flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo flagInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    flagInfo.bindingCount = 1;
    flagInfo.pBindingFlags = &flags;
    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    layoutInfo.pNext = &flagInfo;
    if(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_layout) != VK_SUCCESS)
        return false;
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxAtlases};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = 1;
    pool.poolSizeCount = 1;
    pool.pPoolSizes = &size;
    pool.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    VkDescriptorPool descriptorPool;
    if(vkCreateDescriptorPool(device, &pool, nullptr, &descriptorPool) != VK_SUCCESS)
        return false;
    VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc.descriptorPool = descriptorPool;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts = &m_layout;
    if(vkAllocateDescriptorSets(device, &alloc, &m_set) != VK_SUCCESS)
        return false;
    return true;
}

void DescriptorAtlas::update(VkDevice device, uint32_t index, const VkDescriptorImageInfo& info)
{
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = m_set;
    write.dstBinding = 0;
    write.dstArrayElement = index;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &info;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

} // namespace fallout
