#pragma once
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <fstream>

namespace fallout {

struct AllocatedBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
    VkDeviceSize size = 0;
    VkBufferUsageFlags usage = 0;
    void* mapped = nullptr;
    std::string debugName;
    uint64_t allocationId = 0;
};

struct AllocatedImage {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
    VkImageView view = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent3D extent = {};
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    VkImageUsageFlags usage = 0;
    std::string debugName;
    uint64_t allocationId = 0;
};

enum class MemoryUsagePattern {
    STATIC_GEOMETRY,
    DYNAMIC_UNIFORM,
    STAGING_UPLOAD,
    GPU_ONLY_TEXTURES,
    READBACK_BUFFER,
    PERSISTENT_MAPPED
};

struct MemoryPoolStats {
    VkDeviceSize totalAllocated = 0;
    VkDeviceSize totalUsed = 0;
    uint32_t allocationCount = 0;
    VkDeviceSize largestFreeBlock = 0;
    float fragmentationRatio = 0.0f;
};

class FalloutMemoryManager {
private:
    VmaAllocator allocator = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    std::unordered_map<MemoryUsagePattern, VmaPool> memoryPools;
    uint64_t nextAllocationId = 1;
    std::unordered_map<uint64_t, AllocatedBuffer> bufferAllocations;
    std::unordered_map<uint64_t, AllocatedImage> imageAllocations;
    mutable MemoryPoolStats globalStats;
    mutable std::unordered_map<MemoryUsagePattern, MemoryPoolStats> poolStats;

    struct StagingBuffer {
        AllocatedBuffer buffer;
        VkDeviceSize offset = 0;
        bool inUse = false;
    };
    std::vector<StagingBuffer> stagingBuffers;
    VkDeviceSize stagingBufferSize = 64 * 1024 * 1024;

public:
    bool initialize(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice dev);
    uint64_t createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, MemoryUsagePattern pattern, const std::string& debugName = "");
    uint64_t createImage(VkExtent3D extent, VkFormat format, VkImageUsageFlags usage,
                        uint32_t mipLevels = 1, uint32_t arrayLayers = 1,
                        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
                        MemoryUsagePattern pattern = MemoryUsagePattern::GPU_ONLY_TEXTURES,
                        const std::string& debugName = "");
    bool uploadBufferData(uint64_t bufferId, const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
    MemoryPoolStats getGlobalStats() const;
    MemoryPoolStats getPoolStats(MemoryUsagePattern pattern) const;
    void generateMemoryReport(const std::string& filename) const;
    AllocatedBuffer* getBuffer(uint64_t id);
    AllocatedImage* getImage(uint64_t id);
    void destroyBuffer(uint64_t id);
    void destroyImage(uint64_t id);
    void shutdown();

private:
    bool createMemoryPools();
    bool createMemoryPool(MemoryUsagePattern pattern, VmaMemoryUsage usage, VkDeviceSize size);
    VmaMemoryUsage getVmaUsage(MemoryUsagePattern pattern);
    VmaAllocationCreateFlags getVmaFlags(MemoryUsagePattern pattern);
    VmaPool getMemoryPool(MemoryUsagePattern pattern);
    bool shouldAutoMap(MemoryUsagePattern pattern);
    uint32_t findMemoryType(VmaMemoryUsage usage);
    bool createStagingBuffers();
    bool uploadViaStagingBuffer(VkBuffer dstBuffer, const void* data, VkDeviceSize size, VkDeviceSize offset);
    VkImageAspectFlags getImageAspectMask(VkFormat format);
    void setBufferDebugName(VkBuffer buffer, const std::string& name);
    void setImageDebugName(VkImage image, const std::string& name);
    void setImageViewDebugName(VkImageView view, const std::string& name);
    void updateGlobalStats() const;
    void updatePoolStats() const;
    std::string memoryPatternToString(MemoryUsagePattern pattern) const;
    std::string formatBytes(VkDeviceSize bytes) const;
    std::string getCurrentTimeString() const;
    static void debugAllocationCallback(VmaAllocator allocator, uint32_t memoryType,
                                       VkDeviceMemory memory, VkDeviceSize size, void* pUserData);
    static void debugFreeCallback(VmaAllocator allocator, uint32_t memoryType,
                                 VkDeviceMemory memory, VkDeviceSize size, void* pUserData);
};

} // namespace fallout

