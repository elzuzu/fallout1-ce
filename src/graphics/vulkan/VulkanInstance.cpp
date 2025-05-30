#include "graphics/vulkan/VulkanInstance.h"

#include <SDL_vulkan.h>
#include <cstring>

namespace fallout {

VulkanInstance::~VulkanInstance() {
    shutdown();
}

bool VulkanInstance::initialize(SDL_Window* window, bool enableValidation) {
    unsigned int extensionCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr)) {
        return false;
    }
    std::vector<const char*> extensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());

#ifndef NDEBUG
    if (enableValidation) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
#else
    (void)enableValidation;
#endif

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Fallout Community Edition";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 1, 0);
    appInfo.pEngineName = "Fallout CE Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

#ifndef NDEBUG
    if (enableValidation) {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
#endif

    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
        return false;
    }

#ifndef NDEBUG
    if (enableValidation) {
        setup_debug_messenger(createInfo);
    }
#endif

    return true;
}

void VulkanInstance::shutdown() {
    if (instance_ != VK_NULL_HANDLE) {
#ifndef NDEBUG
        if (debugMessenger_ != VK_NULL_HANDLE) {
            auto destroyFunc = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
            if (destroyFunc) {
                destroyFunc(instance_, debugMessenger_, nullptr);
            }
        }
#endif
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

bool VulkanInstance::setup_debug_messenger(const VkInstanceCreateInfo& createInfo) {
    auto createFunc = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));
    if (!createFunc) {
        return false;
    }
    VkDebugUtilsMessengerCreateInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = nullptr;
    return createFunc(instance_, &info, nullptr, &debugMessenger_) == VK_SUCCESS;
}

} // namespace fallout

