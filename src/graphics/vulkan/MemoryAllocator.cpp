#include "MemoryAllocator.hpp"

VmaAllocator MemoryAllocator::s_alloc { VK_NULL_HANDLE };

void MemoryAllocator::init(VkInstance i, VkPhysicalDevice p, VkDevice d, uint32_t qf)
{
    // Placeholder initialization using VMA
    VmaVulkanFunctions funcs {};
    funcs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    funcs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    VmaAllocatorCreateInfo info {};
    info.instance = i;
    info.physicalDevice = p;
    info.device = d;
    info.vulkanFunctions = funcs;
    vmaCreateAllocator(&info, &s_alloc);
}

void MemoryAllocator::shutdown()
{
    if (s_alloc)
        vmaDestroyAllocator(s_alloc);
    s_alloc = VK_NULL_HANDLE;
}

VmaAllocator MemoryAllocator::handle()
{
    return s_alloc;
}

VmaAllocation MemoryAllocator::createBuffer(VkBufferCreateInfo& bufInfo, VkBuffer& buffer, VmaMemoryUsage usage)
{
    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = usage;
    VmaAllocation alloc = nullptr;
    vmaCreateBuffer(s_alloc, &bufInfo, &allocInfo, &buffer, &alloc, nullptr);
    return alloc;
}

VmaAllocation MemoryAllocator::createImage(VkImageCreateInfo& imgInfo, VkImage& image, VmaMemoryUsage usage)
{
    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = usage;
    VmaAllocation alloc = nullptr;
    vmaCreateImage(s_alloc, &imgInfo, &allocInfo, &image, &alloc, nullptr);
    return alloc;
}
