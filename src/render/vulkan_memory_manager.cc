#include "render/vulkan_memory_manager.h"

#include <cstring>

namespace fallout {

bool VulkanMemoryManager::init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
    m_device = device;
    VmaAllocatorCreateInfo createInfo{};
    createInfo.instance = instance;
    createInfo.physicalDevice = physicalDevice;
    createInfo.device = device;
    createInfo.vulkanApiVersion = VK_API_VERSION_1_1;

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    VkPhysicalDeviceBufferDeviceAddressFeatures addrFeatures{};
    addrFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    features2.pNext = &addrFeatures;
    vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
    if (addrFeatures.bufferDeviceAddress)
        createInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    return vmaCreateAllocator(&createInfo, &m_allocator) == VK_SUCCESS;
}

void VulkanMemoryManager::destroy()
{
    if (m_allocator != VK_NULL_HANDLE)
        vmaDestroyAllocator(m_allocator);
    m_allocator = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
}

void VulkanMemoryManager::uploadDataToBuffer(VulkanBuffer& buffer, const void* data, VkDeviceSize size)
{
    if (!data || size == 0)
        return;
    void* dst = buffer.mappedData;
    if (!dst) {
        vmaMapMemory(m_allocator, buffer.allocation, &dst);
    }
    std::memcpy(dst, data, static_cast<size_t>(size));
    if (!buffer.mappedData)
        vmaUnmapMemory(m_allocator, buffer.allocation);
}

VulkanBuffer VulkanMemoryManager::createVertexBuffer(const void* data, VkDeviceSize size)
{
    VulkanBuffer buf{};
    buf.size = size;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo ainfo{};
    ainfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    ainfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    if (vmaCreateBuffer(m_allocator, &info, &ainfo, &buf.buffer, &buf.allocation, &buf.allocInfo) != VK_SUCCESS)
        return {};
    if (data)
        uploadDataToBuffer(buf, data, size);
    return buf;
}

VulkanBuffer VulkanMemoryManager::createUniformBuffer(VkDeviceSize size)
{
    VulkanBuffer buf{};
    buf.size = size;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo ainfo{};
    ainfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    ainfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    if (vmaCreateBuffer(m_allocator, &info, &ainfo, &buf.buffer, &buf.allocation, &buf.allocInfo) != VK_SUCCESS)
        return {};
    buf.mappedData = buf.allocInfo.pMappedData;
    return buf;
}

VulkanBuffer VulkanMemoryManager::createStagingBuffer(VkDeviceSize size)
{
    VulkanBuffer buf{};
    buf.size = size;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo ainfo{};
    ainfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    ainfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    if (vmaCreateBuffer(m_allocator, &info, &ainfo, &buf.buffer, &buf.allocation, &buf.allocInfo) != VK_SUCCESS)
        return {};
    buf.mappedData = buf.allocInfo.pMappedData;
    return buf;
}

void VulkanMemoryManager::uploadDataToImage(VulkanImage& image, const void* data, size_t sizeBytes)
{
    // Placeholder: real implementation would use staging and command buffers.
    (void)image;
    (void)data;
    (void)sizeBytes;
}

VulkanImage VulkanMemoryManager::createSpriteAtlas(uint32_t width, uint32_t height, const void* data)
{
    VulkanImage img{};
    img.format = VK_FORMAT_R8G8B8A8_UNORM;
    img.extent = {width, height};

    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = img.format;
    info.extent = {width, height, 1};
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo ainfo{};
    ainfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(m_allocator, &info, &ainfo, &img.image, &img.allocation, nullptr) != VK_SUCCESS)
        return {};

    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = img.image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = img.format;
    view.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.layerCount = 1;
    vkCreateImageView(m_device, &view, nullptr, &img.imageView);

    if (data)
        uploadDataToImage(img, data, width * height * 4);

    return img;
}

VulkanImage VulkanMemoryManager::createPaletteTexture(const uint8_t* paletteData, uint32_t paletteCount)
{
    return createSpriteAtlas(256, paletteCount, paletteData);
}

void VulkanMemoryManager::destroyBuffer(VulkanBuffer& buffer)
{
    if (buffer.buffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
    buffer = {};
}

void VulkanMemoryManager::destroyImage(VulkanImage& image)
{
    if (image.imageView != VK_NULL_HANDLE)
        vkDestroyImageView(m_device, image.imageView, nullptr);
    if (image.image != VK_NULL_HANDLE)
        vmaDestroyImage(m_allocator, image.image, image.allocation);
    image = {};
}

ManagedBuffer::~ManagedBuffer()
{
    if (m_buffer.buffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(m_allocator, m_buffer.buffer, m_buffer.allocation);
}

ManagedImage::~ManagedImage()
{
    if (m_image.imageView != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE)
        vkDestroyImageView(m_device, m_image.imageView, nullptr);
    if (m_image.image != VK_NULL_HANDLE)
        vmaDestroyImage(m_allocator, m_image.image, m_image.allocation);
}

} // namespace fallout

