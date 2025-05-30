#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct FrameUBO { float viewProj[16]; float tint[4]; };

static VkBuffer frameUBO = VK_NULL_HANDLE;
static VmaAllocation frameAlloc = nullptr;
static void* mapped = nullptr;
static VkDeviceAddress gpuAddr = 0;

VkResult initFrameUBO(VkDevice device, VmaAllocator alloc)
{
    VkBufferCreateInfo buf{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buf.size = sizeof(FrameUBO);
    buf.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VmaAllocationCreateInfo ci{}; ci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; ci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationInfo info{};
    if(vmaCreateBuffer(alloc,&buf,&ci,&frameUBO,&frameAlloc,&info)!=VK_SUCCESS)
        return VK_ERROR_MEMORY_MAP_FAILED;
    mapped = info.pMappedData;
    VkBufferDeviceAddressInfo addr{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    addr.buffer = frameUBO;
    gpuAddr = vkGetBufferDeviceAddress(device,&addr);
    return VK_SUCCESS;
}

void shutdownFrameUBO(VmaAllocator alloc,VkDevice device)
{
    if(frameUBO) vmaDestroyBuffer(alloc,frameUBO,frameAlloc);
    frameUBO = VK_NULL_HANDLE; frameAlloc=nullptr; mapped=nullptr; gpuAddr=0;
}

