#include "graphics/shader_manager.h"
#include <filesystem>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <iostream>

namespace fallout {

bool ShaderManager::watch(const std::string& path)
{
    FileInfo info{};
    info.last = std::filesystem::last_write_time(path);
    files[path] = info;
    return true;
}

void ShaderManager::poll(VkDevice device)
{
    for (auto& [path, info] : files) {
        auto t = std::filesystem::last_write_time(path);
        if (t != info.last) {
            info.last = t;
            std::vector<char> code;
            std::ifstream f(path, std::ios::binary);
            if (f) {
                code.assign(std::istreambuf_iterator<char>(f), {});
            }
            VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            ci.codeSize = code.size();
            ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
            VkShaderModule mod;
            if (vkCreateShaderModule(device, &ci, nullptr, &mod) == VK_SUCCESS) {
                if (info.module)
                    vkDestroyShaderModule(device, info.module, nullptr);
                info.module = mod;
                std::cout << "[Shader] Reloaded " << path << "\n";
            } else {
                std::cout << "[Shader] Failed to compile " << path << "\n";
            }
        }
    }
}

} // namespace fallout
