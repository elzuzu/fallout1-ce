#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace fallout {
class ShaderManager {
public:
    bool watch(const std::string& path);
    void poll(VkDevice device);
private:
    struct FileInfo { std::filesystem::file_time_type last; VkShaderModule module = VK_NULL_HANDLE; };
    std::unordered_map<std::string, FileInfo> files;
};
}
