#include "PipelineCache.hpp"
#include <fstream>
#include <vector>
#include <filesystem>

std::string PipelineCache::s_path;
VkPipelineCache PipelineCache::s_cache = VK_NULL_HANDLE;
VkDevice PipelineCache::s_device = VK_NULL_HANDLE;

void PipelineCache::init(VkDevice device, const std::string& path)
{
    s_device = device;
    s_path = path;

    std::vector<char> data;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        size_t size = static_cast<size_t>(file.tellg());
        file.seekg(0);
        data.resize(size);
        file.read(data.data(), size);
    } else {
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    }

    VkPipelineCacheCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    info.initialDataSize = data.size();
    info.pInitialData = data.empty() ? nullptr : data.data();
    vkCreatePipelineCache(device, &info, nullptr, &s_cache);
}

void PipelineCache::shutdown()
{
    if (s_cache == VK_NULL_HANDLE || s_device == VK_NULL_HANDLE)
        return;

    size_t size = 0;
    vkGetPipelineCacheData(s_device, s_cache, &size, nullptr);
    std::vector<char> data(size);
    vkGetPipelineCacheData(s_device, s_cache, &size, data.data());

    std::ofstream file(s_path, std::ios::binary | std::ios::trunc);
    if (file.is_open())
        file.write(data.data(), size);

    vkDestroyPipelineCache(s_device, s_cache, nullptr);
    s_cache = VK_NULL_HANDLE;
    s_device = VK_NULL_HANDLE;
}

VkPipelineCache PipelineCache::handle()
{
    return s_cache;
}
