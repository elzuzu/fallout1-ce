#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdarg>
#include <windows.h>

namespace fallout {

enum class RenderBackend {
    AUTO,
    VULKAN_ONLY,
    SDL_ONLY
};

struct VulkanContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    uint32_t graphicsFamily = UINT32_MAX;
    uint32_t presentFamily = UINT32_MAX;
    bool initialized = false;
    std::string lastError;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    bool isComplete() const { return graphics && present; }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

inline void logInfo(const char* fmt, ...) {
    va_list args; va_start(args, fmt); vfprintf(stdout, fmt, args); va_end(args); fprintf(stdout, "\n");
}
inline void logWarning(const char* fmt, ...) {
    va_list args; va_start(args, fmt); vfprintf(stderr, fmt, args); va_end(args); fprintf(stderr, "\n");
}

class VulkanManager {
private:
    VulkanContext context{};
    HWND windowHandle{};

public:
    enum class InitResult {
        SUCCESS,
        VULKAN_NOT_SUPPORTED,
        NO_SUITABLE_DEVICE,
        SURFACE_CREATION_FAILED,
        DEVICE_CREATION_FAILED,
        UNKNOWN_ERROR
    };

    InitResult initialize(HWND hwnd, bool enableValidation = false) {
        windowHandle = hwnd;

        uint32_t instanceVersion = 0;
        VkResult result = vkEnumerateInstanceVersion(&instanceVersion);
        if (result != VK_SUCCESS) {
            context.lastError = "Vulkan runtime not found on system";
            return InitResult::VULKAN_NOT_SUPPORTED;
        }
        if (instanceVersion < VK_MAKE_VERSION(1, 1, 0)) {
            context.lastError = std::string("Vulkan 1.1+ required, found ") + versionToString(instanceVersion);
            return InitResult::VULKAN_NOT_SUPPORTED;
        }
        if (!createInstance(enableValidation)) {
            return InitResult::VULKAN_NOT_SUPPORTED;
        }
        if (enableValidation && !setupDebugMessenger()) {
            logWarning("Debug messenger setup failed, continuing without validation");
        }
        if (!createSurface()) {
            cleanup();
            return InitResult::SURFACE_CREATION_FAILED;
        }
        if (!selectPhysicalDevice()) {
            cleanup();
            return InitResult::NO_SUITABLE_DEVICE;
        }
        if (!createLogicalDevice()) {
            cleanup();
            return InitResult::DEVICE_CREATION_FAILED;
        }
        context.initialized = true;
        logInfo("Vulkan initialized successfully");
        return InitResult::SUCCESS;
    }

    const VulkanContext& getContext() const { return context; }

private:
    bool createInstance(bool enableValidation) {
        std::vector<const char*> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        };
        if (enableValidation) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        if (!checkExtensionSupport(extensions)) {
            context.lastError = "Required Vulkan extensions not available";
            return false;
        }
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Fallout1-CE";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Fallout1-CE";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        std::vector<const char*> validationLayers;
        if (enableValidation && checkValidationLayerSupport()) {
            validationLayers.push_back("VK_LAYER_KHRONOS_validation");
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }

        VkResult result = vkCreateInstance(&createInfo, nullptr, &context.instance);
        if (result != VK_SUCCESS) {
            context.lastError = std::string("Failed to create Vulkan instance: ") + resultToString(result);
            return false;
        }
        return true;
    }

    bool setupDebugMessenger() {
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT"));
        if (!func) return false;
        VkDebugUtilsMessengerCreateInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback = nullptr;
        return func(context.instance, &info, nullptr, &context.debugMessenger) == VK_SUCCESS;
    }

    bool createSurface() {
        VkWin32SurfaceCreateInfoKHR ci{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
        ci.hinstance = GetModuleHandle(nullptr);
        ci.hwnd = windowHandle;
        return vkCreateWin32SurfaceKHR(context.instance, &ci, nullptr, &context.surface) == VK_SUCCESS;
    }

    bool selectPhysicalDevice() {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(context.instance, &count, nullptr);
        if (count == 0) {
            context.lastError = "No Vulkan-capable devices found";
            return false;
        }
        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(context.instance, &count, devices.data());
        int bestScore = -1;
        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
        for (auto dev : devices) {
            int score = scoreDeviceForFallout(dev);
            if (score > bestScore) {
                bestScore = score;
                bestDevice = dev;
            }
        }
        if (bestDevice == VK_NULL_HANDLE) {
            context.lastError = "No suitable Vulkan device found for Fallout1-CE";
            return false;
        }
        context.physicalDevice = bestDevice;
        return true;
    }

    bool createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(context.physicalDevice);
        if (!indices.isComplete()) {
            context.lastError = "Required queue families not found";
            return false;
        }
        float priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        std::vector<uint32_t> unique = {indices.graphics.value(), indices.present.value()};
        if (unique[0] != unique[1]) {
            unique.push_back(unique[1]);
        }
        std::sort(unique.begin(), unique.end());
        unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
        for (uint32_t fam : unique) {
            VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            q.queueFamilyIndex = fam;
            q.queueCount = 1;
            q.pQueuePriorities = &priority;
            queueInfos.push_back(q);
        }
        VkPhysicalDeviceFeatures features{};
        features.samplerAnisotropy = VK_TRUE;
        features.fillModeNonSolid = VK_TRUE;
        features.multiViewport = VK_TRUE;

        const char* deviceExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        info.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
        info.pQueueCreateInfos = queueInfos.data();
        info.pEnabledFeatures = &features;
        info.enabledExtensionCount = 1;
        info.ppEnabledExtensionNames = deviceExts;

        VkResult res = vkCreateDevice(context.physicalDevice, &info, nullptr, &context.device);
        if (res != VK_SUCCESS) {
            context.lastError = std::string("Failed to create logical device: ") + resultToString(res);
            return false;
        }
        context.graphicsFamily = indices.graphics.value();
        context.presentFamily = indices.present.value();
        vkGetDeviceQueue(context.device, context.graphicsFamily, 0, &context.graphicsQueue);
        vkGetDeviceQueue(context.device, context.presentFamily, 0, &context.presentQueue);
        return true;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());
        for (uint32_t i = 0; i < count; ++i) {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphics = i;
            }
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context.surface, &present);
            if (present) {
                indices.present = i;
            }
            if (indices.isComplete()) break;
        }
        return indices;
    }

    bool checkExtensionSupport(const std::vector<const char*>& extensions) {
        uint32_t count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        std::vector<VkExtensionProperties> available(count);
        vkEnumerateInstanceExtensionProperties(nullptr, &count, available.data());
        for (const char* name : extensions) {
            bool found = false;
            for (const auto& ext : available) {
                if (strcmp(name, ext.extensionName) == 0) {
                    found = true; break;
                }
            }
            if (!found) return false;
        }
        return true;
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
        for (const auto& layer : layers) {
            if (strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) return true;
        }
        return false;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> available(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, available.data());
        std::set<std::string> required = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        for (const auto& ext : available) {
            required.erase(ext.extensionName);
        }
        return required.empty();
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context.surface, &details.capabilities);
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &formatCount, details.formats.data());
        }
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.surface, &presentModeCount, details.presentModes.data());
        }
        return details;
    }

    int scoreDeviceForFallout(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceProperties(device, &properties);
        vkGetPhysicalDeviceFeatures(device, &features);
        int score = 0;
        if (!features.samplerAnisotropy) return 0;
        if (!features.fillModeNonSolid) return 0;
        if (!features.multiViewport) return 0;
        QueueFamilyIndices indices = findQueueFamilies(device);
        if (!indices.isComplete()) return 0;
        if (!checkDeviceExtensionSupport(device)) return 0;
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) return 0;
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }
        score += properties.limits.maxImageDimension2D / 1000;
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryHeapCount; ++i) {
            if (memProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                score += static_cast<int>(memProperties.memoryHeaps[i].size / (1024 * 1024));
            }
        }
        return score;
    }

    void cleanup() {
        if (context.device != VK_NULL_HANDLE) {
            vkDestroyDevice(context.device, nullptr);
        }
        if (context.surface != VK_NULL_HANDLE && context.instance != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
        }
        if (context.debugMessenger != VK_NULL_HANDLE) {
            auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT"));
            if (func) func(context.instance, context.debugMessenger, nullptr);
        }
        if (context.instance != VK_NULL_HANDLE) {
            vkDestroyInstance(context.instance, nullptr);
        }
        context = VulkanContext{};
    }

    std::string versionToString(uint32_t version) {
        return std::to_string(VK_VERSION_MAJOR(version)) + "." +
               std::to_string(VK_VERSION_MINOR(version)) + "." +
               std::to_string(VK_VERSION_PATCH(version));
    }

    std::string resultToString(VkResult res) {
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
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        default: return "UNKNOWN";
        }
    }
};

} // namespace fallout

