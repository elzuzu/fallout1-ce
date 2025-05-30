#pragma once
#include <vulkan/vulkan.h>
#include <string>

class PipelineCache {
public:
    static void init(VkDevice device, const std::string& path = "cache/pipeline.bin");
    static void shutdown();
    static VkPipelineCache handle();
private:
    static std::string     s_path;
    static VkPipelineCache s_cache;
    static VkDevice        s_device;
};
