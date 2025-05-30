#include "render/vulkan_memory_manager.h"

#include <cstring>

namespace fallout {

bool VulkanMemoryManager::init(VkPhysicalDevice physicalDevice, VkDevice device)
{
    m_physicalDevice = physicalDevice;
    m_device = device;

    const VkDeviceSize vertexSize = 64 * 1024 * 1024;
    const VkDeviceSize indexSize = 32 * 1024 * 1024;
    const VkDeviceSize uniformSize = 16 * 1024 * 1024;

    if (!init_pool(m_vertexPool, vertexSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        return false;
    if (!init_pool(m_indexPool, indexSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        return false;
    if (!init_pool(m_uniformPool, uniformSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        return false;

    return true;
}

void VulkanMemoryManager::destroy()
{
    if (m_device == VK_NULL_HANDLE)
        return;

    if (m_vertexPool.memory != VK_NULL_HANDLE)
        vkFreeMemory(m_device, m_vertexPool.memory, nullptr);
    if (m_indexPool.memory != VK_NULL_HANDLE)
        vkFreeMemory(m_device, m_indexPool.memory, nullptr);
    if (m_uniformPool.memory != VK_NULL_HANDLE)
        vkFreeMemory(m_device, m_uniformPool.memory, nullptr);

    m_vertexPool = {};
    m_indexPool = {};
    m_uniformPool = {};
    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
}

uint32_t VulkanMemoryManager::find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    return 0;
}

bool VulkanMemoryManager::init_pool(MemoryPool& pool, VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo bufInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufInfo.size = size;
    bufInfo.usage = usage;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer tmpBuffer = VK_NULL_HANDLE;
    if (vkCreateBuffer(m_device, &bufInfo, nullptr, &tmpBuffer) != VK_SUCCESS)
        return false;

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(m_device, tmpBuffer, &req);
    vkDestroyBuffer(m_device, tmpBuffer, nullptr);

    VkMemoryAllocateInfo allocInfo { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = size;
    allocInfo.memoryTypeIndex = find_memory_type(req.memoryTypeBits, properties);
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &pool.memory) != VK_SUCCESS)
        return false;

    pool.size = size;
    pool.offset = 0;
    return true;
}

bool VulkanMemoryManager::allocate_buffer(PoolType type, VkDeviceSize size, VkBufferUsageFlags usage, Allocation& out)
{
    MemoryPool* pool = nullptr;
    switch (type) {
    case PoolType::Vertex:
        pool = &m_vertexPool;
        break;
    case PoolType::Index:
        pool = &m_indexPool;
        break;
    case PoolType::Uniform:
        pool = &m_uniformPool;
        break;
    }

    if (pool == nullptr || pool->memory == VK_NULL_HANDLE)
        return false;

    VkBufferCreateInfo bufferInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &out.buffer) != VK_SUCCESS)
        return false;

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(m_device, out.buffer, &req);
    VkDeviceSize alignedOffset = (pool->offset + req.alignment - 1) & ~(req.alignment - 1);

    if (alignedOffset + req.size > pool->size) {
        vkDestroyBuffer(m_device, out.buffer, nullptr);
        out.buffer = VK_NULL_HANDLE;
        return false;
    }

    vkBindBufferMemory(m_device, out.buffer, pool->memory, alignedOffset);

    out.memory = pool->memory;
    out.offset = alignedOffset;
    pool->offset = alignedOffset + req.size;
    return true;
}

bool VulkanMemoryManager::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory)
{
    VkBufferCreateInfo bufferInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        return false;

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(m_device, buffer, &req);

    VkMemoryAllocateInfo allocInfo { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = req.size;
    allocInfo.memoryTypeIndex = find_memory_type(req.memoryTypeBits, properties);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, buffer, nullptr);
        return false;
    }

    vkBindBufferMemory(m_device, buffer, memory, 0);
    return true;
}

bool VulkanMemoryManager::create_texture_with_staging(const void* pixels, VkDeviceSize size,
    const VkImageCreateInfo& imageInfo, VkCommandPool commandPool, VkQueue queue,
    VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imgInfo = imageInfo;
    if (vkCreateImage(m_device, &imgInfo, nullptr, &image) != VK_SUCCESS)
        return false;

    VkMemoryRequirements imgReq;
    vkGetImageMemoryRequirements(m_device, image, &imgReq);

    VkMemoryAllocateInfo allocInfo { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = imgReq.size;
    allocInfo.memoryTypeIndex = find_memory_type(imgReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        vkDestroyImage(m_device, image, nullptr);
        return false;
    }
    vkBindImageMemory(m_device, image, imageMemory, 0);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    if (!create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingMemory)) {
        vkDestroyImage(m_device, image, nullptr);
        vkFreeMemory(m_device, imageMemory, nullptr);
        return false;
    }

    void* data = nullptr;
    vkMapMemory(m_device, stagingMemory, 0, size, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(size));
    vkUnmapMemory(m_device, stagingMemory);

    VkCommandBufferAllocateInfo cmdInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmdInfo.commandPool = commandPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_device, &cmdInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferImageCopy region {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = imageInfo.extent.width;
    region.imageExtent.height = imageInfo.extent.height;
    region.imageExtent.depth = 1;

    VkImageMemoryBarrier barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkCmdCopyBufferToImage(cmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(m_device, commandPool, 1, &cmd);
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    return true;
}

} // namespace fallout
