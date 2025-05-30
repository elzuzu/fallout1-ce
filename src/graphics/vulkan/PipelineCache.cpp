#include "PipelineCache.hpp"
#include <filesystem>
#include <memory>
#include <vector>
#include <cstdio>

std::string PipelineCache::s_path;
VkPipelineCache PipelineCache::s_cache = VK_NULL_HANDLE;
VkDevice PipelineCache::s_device = VK_NULL_HANDLE;

void PipelineCache::init(VkDevice device, const std::string& path)
{
    s_device = device;
    s_path = path;

    std::vector<char> data;
    if (std::filesystem::exists(path)) {
        std::unique_ptr<std::FILE, decltype(&std::fclose)> f(std::fopen(path.c_str(), "rb"), &std::fclose);
        if (f) {
            std::fseek(f.get(), 0, SEEK_END);
            size_t size = std::ftell(f.get());
            std::rewind(f.get());
            data.resize(size);
            if (std::fread(data.data(), 1, size, f.get()) != size) {
                return;
            }
        }
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

    std::unique_ptr<std::FILE, decltype(&std::fclose)> f(std::fopen(s_path.c_str(), "wb"), &std::fclose);
    if (f)
        std::fwrite(data.data(), 1, size, f.get());

    vkDestroyPipelineCache(s_device, s_cache, nullptr);
    s_cache = VK_NULL_HANDLE;
    s_device = VK_NULL_HANDLE;
}

VkPipelineCache PipelineCache::handle()
{
    return s_cache;
}
