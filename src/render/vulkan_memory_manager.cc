#include "render/vulkan_memory_manager.h"
#include "plib/gnw/debug.h"

#include <cstring>

#ifndef NDEBUG
static void setBufferDebugName(VkDevice device, VkBuffer buffer, const std::string& name)
{
    auto func = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
        vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
    if (func && buffer != VK_NULL_HANDLE) {
        VkDebugUtilsObjectNameInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
        info.objectType = VK_DEBUG_UTILS_OBJECT_TYPE_BUFFER_EXT;
        info.objectHandle = reinterpret_cast<uint64_t>(buffer);
        info.pObjectName = name.c_str();
        func(device, &info);
    }
}

static void setImageDebugName(VkDevice device, VkImage image, const std::string& name)
{
    auto func = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
        vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
    if (func && image != VK_NULL_HANDLE) {
        VkDebugUtilsObjectNameInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
        info.objectType = VK_DEBUG_UTILS_OBJECT_TYPE_IMAGE_EXT;
        info.objectHandle = reinterpret_cast<uint64_t>(image);
        info.pObjectName = name.c_str();
        func(device, &info);
    }
}
#else
static void setBufferDebugName(VkDevice, VkBuffer, const std::string&) {}
static void setImageDebugName(VkDevice, VkImage, const std::string&) {}
#endif

static const char* translateVulkanError(VkResult res)
{
    switch (res) {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_NOT_READY: return "VK_NOT_READY";
    case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_EVENT_SET: return "VK_EVENT_SET";
    case VK_EVENT_RESET: return "VK_EVENT_RESET";
    case VK_INCOMPLETE: return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
    default: return "UNKNOWN_ERROR";
    }
}

#define logInfo(...) fallout::debug_printf(__VA_ARGS__)
#define logError(...) fallout::debug_printf(__VA_ARGS__)

namespace fallout {

bool VulkanMemoryManager::init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
    m_device = device;
    m_physicalDevice = physicalDevice;

    VmaAllocatorCreateInfo createInfo{};
    createInfo.instance = instance;
    createInfo.physicalDevice = physicalDevice;
    createInfo.device = device;
    createInfo.vulkanApiVersion = VK_API_VERSION_1_1;
    createInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    VkPhysicalDeviceBufferDeviceAddressFeatures addrFeatures{};
    addrFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    features2.pNext = &addrFeatures;
    vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

    if (addrFeatures.bufferDeviceAddress)
        createInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    VkResult result = vmaCreateAllocator(&createInfo, &m_allocator);
    if (result != VK_SUCCESS) {
        logError("Failed to create VMA allocator: %s", translateVulkanError(result));
        return false;
    }

    logInfo("VMA memory allocator initialized\n");
    return true;
}

void VulkanMemoryManager::destroy()
{
    if (m_allocator != VK_NULL_HANDLE) {
        printMemoryStats();
        vmaDestroyAllocator(m_allocator);
    }
    m_allocator = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
}

void VulkanMemoryManager::printMemoryStats()
{
    if (m_allocator == VK_NULL_HANDLE)
        return;

    VmaStats stats{};
    vmaCalculateStats(m_allocator, &stats);
    logInfo("VMA Memory Statistics:\n");
    logInfo("- Total allocated: %.2f MB\n", stats.total.usedBytes / (1024.0f * 1024.0f));
    logInfo("- Total available: %.2f MB\n", stats.total.unusedBytes / (1024.0f * 1024.0f));
    logInfo("- Allocation count: %u\n", stats.total.allocationCount);
    logInfo("- Memory blocks: %u\n", stats.total.blockCount);
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

VulkanBuffer VulkanMemoryManager::createVertexBuffer(const void* data, VkDeviceSize size, const std::string& name)
{
    VulkanBuffer buf{};
    buf.size = size;
    buf.debugName = name.empty() ? "VertexBuffer" : name;

    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo ainfo{};
    ainfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    ainfo.flags = 0;
    if (size > 1024 * 1024)
        ainfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VkResult result = vmaCreateBuffer(m_allocator, &info, &ainfo, &buf.buffer, &buf.allocation, &buf.allocInfo);
    if (result != VK_SUCCESS) {
        logError("Failed to create vertex buffer '%s': %s", buf.debugName.c_str(), translateVulkanError(result));
        return {};
    }

    setBufferDebugName(m_device, buf.buffer, buf.debugName);

    if (data)
        uploadDataToBuffer(buf, data, size);

    m_totalAllocatedBytes += buf.allocInfo.size;
    m_bufferNames[buf.buffer] = buf.debugName;

    return buf;
}

VulkanBuffer VulkanMemoryManager::createIndexBuffer(const void* data, VkDeviceSize size, const std::string& name)
{
    VulkanBuffer buf{};
    buf.size = size;
    buf.debugName = name.empty() ? "IndexBuffer" : name;

    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo ainfo{};
    ainfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkResult result = vmaCreateBuffer(m_allocator, &info, &ainfo, &buf.buffer, &buf.allocation, &buf.allocInfo);
    if (result != VK_SUCCESS) {
        logError("Failed to create index buffer '%s': %s", buf.debugName.c_str(), translateVulkanError(result));
        return {};
    }

    setBufferDebugName(m_device, buf.buffer, buf.debugName);

    if (data)
        uploadDataToBuffer(buf, data, size);

    m_totalAllocatedBytes += buf.allocInfo.size;
    m_bufferNames[buf.buffer] = buf.debugName;

    return buf;
}

VulkanBuffer VulkanMemoryManager::createUniformBuffer(VkDeviceSize size, bool hostVisible, const std::string& name)
{
    VulkanBuffer buf{};
    buf.size = size;
    buf.debugName = name.empty() ? "UniformBuffer" : name;

    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo ainfo{};
    if (hostVisible) {
        ainfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        ainfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;
    } else {
        ainfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    VkResult result = vmaCreateBuffer(m_allocator, &info, &ainfo, &buf.buffer, &buf.allocation, &buf.allocInfo);
    if (result != VK_SUCCESS) {
        logError("Failed to create uniform buffer '%s': %s", buf.debugName.c_str(), translateVulkanError(result));
        return {};
    }

    setBufferDebugName(m_device, buf.buffer, buf.debugName);

    if (hostVisible && buf.allocInfo.pMappedData)
        buf.mappedData = buf.allocInfo.pMappedData;

    m_totalAllocatedBytes += buf.allocInfo.size;
    m_bufferNames[buf.buffer] = buf.debugName;

    return buf;
}

VulkanBuffer VulkanMemoryManager::createStagingBuffer(VkDeviceSize size, const std::string& name)
{
    VulkanBuffer buf{};
    buf.size = size;
    buf.debugName = name.empty() ? "StagingBuffer" : name;

    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo ainfo{};
    ainfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    ainfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                  VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkResult result = vmaCreateBuffer(m_allocator, &info, &ainfo, &buf.buffer, &buf.allocation, &buf.allocInfo);
    if (result != VK_SUCCESS) {
        logError("Failed to create staging buffer '%s': %s", buf.debugName.c_str(), translateVulkanError(result));
        return {};
    }

    buf.mappedData = buf.allocInfo.pMappedData;
    setBufferDebugName(m_device, buf.buffer, buf.debugName);

    m_totalAllocatedBytes += buf.allocInfo.size;
    m_bufferNames[buf.buffer] = buf.debugName;

    return buf;
}

void VulkanMemoryManager::uploadDataToImage(VulkanImage& image, const void* data, size_t sizeBytes)
{
    // Placeholder: real implementation would use staging and command buffers.
    (void)image;
    (void)data;
    (void)sizeBytes;
}

VulkanImage VulkanMemoryManager::createSpriteAtlas(uint32_t width, uint32_t height, const void* data, const std::string& name)
{
    VulkanImage img{};
    img.format = VK_FORMAT_R8G8B8A8_UNORM;
    img.extent = {width, height};
    img.debugName = name.empty() ? "SpriteAtlas" : name;

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

    VkResult result = vmaCreateImage(m_allocator, &info, &ainfo, &img.image, &img.allocation, nullptr);
    if (result != VK_SUCCESS) {
        logError("Failed to create sprite atlas '%s': %s", img.debugName.c_str(), translateVulkanError(result));
        return {};
    }

    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = img.image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = img.format;
    view.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.layerCount = 1;
    result = vkCreateImageView(m_device, &view, nullptr, &img.imageView);
    if (result != VK_SUCCESS) {
        vmaDestroyImage(m_allocator, img.image, img.allocation);
        logError("Failed to create image view for '%s': %s", img.debugName.c_str(), translateVulkanError(result));
        return {};
    }

    setImageDebugName(m_device, img.image, img.debugName);
    m_imageNames[img.image] = img.debugName;

    if (data)
        uploadDataToImage(img, data, width * height * 4);

    m_totalAllocatedBytes += img.extent.width * img.extent.height * 4; // approximate
    
    return img;
}

VulkanImage VulkanMemoryManager::createPaletteTexture(const uint8_t* paletteData, uint32_t paletteCount, const std::string& name)
{
    return createSpriteAtlas(256, paletteCount, paletteData, name.empty() ? "PaletteTexture" : name);
}

void VulkanMemoryManager::destroyBuffer(VulkanBuffer& buffer)
{
    if (buffer.buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
        m_totalAllocatedBytes -= buffer.allocInfo.size;
        m_bufferNames.erase(buffer.buffer);
    }
    buffer = {};
}

void VulkanMemoryManager::destroyImage(VulkanImage& image)
{
    if (image.imageView != VK_NULL_HANDLE)
        vkDestroyImageView(m_device, image.imageView, nullptr);
    if (image.image != VK_NULL_HANDLE) {
        vmaDestroyImage(m_allocator, image.image, image.allocation);
        m_imageNames.erase(image.image);
    }
    image = {};
}


} // namespace fallout

