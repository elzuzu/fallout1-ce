#ifndef VMA_INCLUDE_VK_MEM_ALLOC_H_
#define VMA_INCLUDE_VK_MEM_ALLOC_H_
#include <vulkan/vulkan.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

typedef enum VmaMemoryUsage {
    VMA_MEMORY_USAGE_UNKNOWN = 0,
    VMA_MEMORY_USAGE_GPU_ONLY = 1,
    VMA_MEMORY_USAGE_CPU_ONLY = 2,
    VMA_MEMORY_USAGE_CPU_TO_GPU = 3,
    VMA_MEMORY_USAGE_GPU_TO_CPU = 4,
} VmaMemoryUsage;

typedef struct VmaVulkanFunctions {
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
} VmaVulkanFunctions;

typedef struct VmaAllocatorCreateInfo {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    uint32_t flags;
    VmaVulkanFunctions vulkanFunctions;
} VmaAllocatorCreateInfo;

typedef struct VmaAllocationCreateInfo {
    VmaMemoryUsage usage;
} VmaAllocationCreateInfo;

static inline VkResult vmaCreateAllocator(const VmaAllocatorCreateInfo* c, VmaAllocator* a)
{
    (void)c;
    *a = (VmaAllocator)1;
    return VK_SUCCESS;
}
static inline void vmaDestroyAllocator(VmaAllocator a) { (void)a; }
static inline VkResult vmaCreateBuffer(VmaAllocator, const VkBufferCreateInfo*, const VmaAllocationCreateInfo*, VkBuffer* b, VmaAllocation* alloc, void*)
{
    *b = VK_NULL_HANDLE;
    *alloc = (VmaAllocation)1;
    return VK_SUCCESS;
}
static inline void vmaDestroyBuffer(VmaAllocator, VkBuffer, VmaAllocation) { }
static inline VkResult vmaCreateImage(VmaAllocator, const VkImageCreateInfo*, const VmaAllocationCreateInfo*, VkImage* img, VmaAllocation* alloc, void*)
{
    *img = VK_NULL_HANDLE;
    *alloc = (VmaAllocation)1;
    return VK_SUCCESS;
}
static inline void vmaDestroyImage(VmaAllocator, VkImage, VmaAllocation) { }

#ifdef __cplusplus
}
#endif
#endif // VMA_INCLUDE_VK_MEM_ALLOC_H_
