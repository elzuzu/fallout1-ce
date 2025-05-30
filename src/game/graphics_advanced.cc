#include "game/graphics_advanced.h"

#include <SDL.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <stdlib.h>
#include <string.h>

#include "game/config.h"
#include "platform_compat.h"
#include "plib/gnw/intrface.h"
#include "plib/gnw/svga.h"

namespace fallout {

GraphicsAdvancedOptions gGraphicsAdvanced{0, 2, 1, true, 60, false, false};

static std::vector<std::string> enumerate_gpus()
{
    std::vector<std::string> names;
    if (SDL_Vulkan_LoadLibrary(nullptr) != 0)
        return names;

    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instInfo.pApplicationInfo = &appInfo;

    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&instInfo, nullptr, &instance) != VK_SUCCESS) {
        SDL_Vulkan_UnloadLibrary();
        return names;
    }

    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count > 0) {
        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(instance, &count, devices.data());
        names.reserve(count);
        for (uint32_t i = 0; i < count; i++) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(devices[i], &props);
            names.emplace_back(props.deviceName);
        }
    }

    vkDestroyInstance(instance, nullptr);
    SDL_Vulkan_UnloadLibrary();
    return names;
}

void graphics_advanced_load()
{
    const char* use_config = getenv("F1CE_USE_CONFIG_FILES");
    if (use_config == NULL || strcmp(use_config, "1") != 0)
        return;

    Config cfg;
    if (!config_init(&cfg))
        return;

    if (config_load(&cfg, "f1_res.ini", false)) {
        config_get_value(&cfg, "MAIN", "GPU_INDEX", &gGraphicsAdvanced.gpuIndex);
        config_get_value(&cfg, "MAIN", "TEXTURE_QUALITY", &gGraphicsAdvanced.textureQuality);
        config_get_value(&cfg, "MAIN", "FILTERING", &gGraphicsAdvanced.filtering);
        configGetBool(&cfg, "MAIN", "VSYNC", &gGraphicsAdvanced.vsync);
        config_get_value(&cfg, "MAIN", "FPS_LIMIT", &gGraphicsAdvanced.fpsLimit);
        configGetBool(&cfg, "MAIN", "VK_VALIDATION", &gGraphicsAdvanced.validation);
        configGetBool(&cfg, "MAIN", "VK_MULTITHREADED", &gGraphicsAdvanced.multithreaded);
    }

    config_exit(&cfg);
}

void graphics_advanced_save()
{
    const char* use_config = getenv("F1CE_USE_CONFIG_FILES");
    if (use_config == NULL || strcmp(use_config, "1") != 0)
        return;

    Config cfg;
    if (!config_init(&cfg))
        return;

    config_load(&cfg, "f1_res.ini", false);
    config_set_value(&cfg, "MAIN", "GPU_INDEX", gGraphicsAdvanced.gpuIndex);
    config_set_value(&cfg, "MAIN", "TEXTURE_QUALITY", gGraphicsAdvanced.textureQuality);
    config_set_value(&cfg, "MAIN", "FILTERING", gGraphicsAdvanced.filtering);
    configSetBool(&cfg, "MAIN", "VSYNC", gGraphicsAdvanced.vsync);
    config_set_value(&cfg, "MAIN", "FPS_LIMIT", gGraphicsAdvanced.fpsLimit);
    configSetBool(&cfg, "MAIN", "VK_VALIDATION", gGraphicsAdvanced.validation);
    configSetBool(&cfg, "MAIN", "VK_MULTITHREADED", gGraphicsAdvanced.multithreaded);
    config_save(&cfg, "f1_res.ini", false);
    config_exit(&cfg);
}

void graphics_advanced_apply()
{
    sharedFpsLimiter.setFps(gGraphicsAdvanced.fpsLimit);
}

int do_graphics_advanced()
{
    std::vector<std::string> gpus = enumerate_gpus();
    std::vector<char*> gpuList;
    for (const auto& n : gpus) {
        gpuList.push_back(const_cast<char*>(n.c_str()));
    }
    if (!gpuList.empty()) {
        int idx = win_list_select("Select GPU", gpuList.data(), gpuList.size(), NULL, 80, 80, 0x10000 | 0x100 | 4);
        if (idx != -1)
            gGraphicsAdvanced.gpuIndex = idx;
    }

    char* qualityItems[] = { (char*)"Low", (char*)"Medium", (char*)"High" };
    int q = win_list_select("Texture Quality", qualityItems, 3, NULL, 80, 80, 0x10000 | 0x100 | 4);
    if (q != -1)
        gGraphicsAdvanced.textureQuality = q;

    char* filterItems[] = { (char*)"Nearest", (char*)"Linear" };
    int f = win_list_select("Filtering", filterItems, 2, NULL, 80, 80, 0x10000 | 0x100 | 4);
    if (f != -1)
        gGraphicsAdvanced.filtering = f;

    char* yesno[] = { (char*)"No", (char*)"Yes" };
    int v = win_list_select("V-sync", yesno, 2, NULL, 80, 80, 0x10000 | 0x100 | 4);
    if (v != -1)
        gGraphicsAdvanced.vsync = v == 1;

    char str[16];
    snprintf(str, sizeof(str), "%d", gGraphicsAdvanced.fpsLimit);
    if (win_get_str(str, sizeof(str), "FPS Limit", 80, 80) == 0) {
        int val = atoi(str);
        if (val > 0)
            gGraphicsAdvanced.fpsLimit = val;
    }

    int d = win_list_select("Validation Layers", yesno, 2, NULL, 80, 80, 0x10000 | 0x100 | 4);
    if (d != -1)
        gGraphicsAdvanced.validation = d == 1;

    int m = win_list_select("Vulkan Multithreaded", yesno, 2, NULL, 80, 80, 0x10000 | 0x100 | 4);
    if (m != -1)
        gGraphicsAdvanced.multithreaded = m == 1;

    graphics_advanced_save();
    graphics_advanced_apply();
    return 0;
}

} // namespace fallout

