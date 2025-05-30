#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

class MemoryAllocator {
public:
    static void init(VkInstance, VkPhysicalDevice, VkDevice, uint32_t queueFamily);
    static void shutdown();
    static VmaAllocator handle();

    // helpers
    static VmaAllocation createBuffer(VkBufferCreateInfo&, VkBuffer&, VmaMemoryUsage);
    static VmaAllocation createImage(VkImageCreateInfo&, VkImage&, VmaMemoryUsage);

private:
    static VmaAllocator s_alloc;
};
