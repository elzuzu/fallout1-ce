#include "graphics/renderer_interface.h"
#include "graphics/vk_core_fix.hpp"
#include "plib/gnw/debug.h"
#include "graphics/vulkan_error_reporter.hpp"
#include "plib/gnw/svga.h"
#include <memory>
#include <algorithm>
#include <SDL_syswm.h>
#include <windows.h>

namespace fallout {

class SDLRenderer; // forward declaration
class VulkanBackendRenderer; // forward declaration

static void saveRendererChoice(const char* /*backend*/) {
    // TODO: persist choice in configuration
}

namespace {

HWND getHWND(SDL_Window* window) {
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info)) {
        return info.info.win.window;
    }
    return nullptr;
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
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

bool checkValidationLayerSupport(const char* layerName) {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    for (const auto& layer : layers) {
        if (strcmp(layer.layerName, layerName) == 0) return true;
    }
    return false;
}

struct QueueFamilies {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    bool isComplete() const { return graphics && present; }
};

QueueFamilies findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilies indices;
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    for (uint32_t i = 0; i < count; ++i) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics = i;
        }
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present);
        if (present) {
            indices.present = i;
        }
        if (indices.isComplete()) break;
    }
    return indices;
}

bool checkSwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount == 0) return false;
    uint32_t presentCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentCount, nullptr);
    return presentCount != 0;
}

int scoreDevice(VkPhysicalDevice device, VkSurfaceKHR surface) {
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device, &props);
    vkGetPhysicalDeviceFeatures(device, &features);

    int score = 0;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
    if (!features.samplerAnisotropy) return 0;
    if (!features.fillModeNonSolid) return 0;
    if (!findQueueFamilies(device, surface).isComplete()) return 0;
    if (!checkSwapchainSupport(device, surface)) return 0;
    score += props.limits.maxImageDimension2D / 1000;
    return score;
}

bool selectBestPhysicalDevice(VulkanInitResult& result) {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(result.instance, &count, nullptr);
    if (count == 0) return false;
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(result.instance, &count, devices.data());
    int bestScore = -1;
    VkPhysicalDevice best = VK_NULL_HANDLE;
    for (const auto& device : devices) {
        int score = scoreDevice(device, result.surface);
        if (score > bestScore) {
            bestScore = score;
            best = device;
        }
    }
    if (best == VK_NULL_HANDLE) return false;
    result.physicalDevice = best;
    return true;
}

bool createLogicalDevice(VulkanInitResult& result) {
    QueueFamilies indices = findQueueFamilies(result.physicalDevice, result.surface);
    if (!indices.isComplete()) return false;
    float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queues;
    std::vector<uint32_t> unique = { indices.graphics.value(), indices.present.value() };
    std::sort(unique.begin(), unique.end());
    unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
    for (uint32_t fam : unique) {
        VkDeviceQueueCreateInfo q{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        q.queueFamilyIndex = fam;
        q.queueCount = 1;
        q.pQueuePriorities = &priority;
        queues.push_back(q);
    }
    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;
    features.fillModeNonSolid = VK_TRUE;
    VkDeviceCreateInfo info{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    info.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
    info.pQueueCreateInfos = queues.data();
    info.pEnabledFeatures = &features;
    const char* exts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    info.enabledExtensionCount = 1;
    info.ppEnabledExtensionNames = exts;
    if (vkCreateDevice(result.physicalDevice, &info, nullptr, &result.device) != VK_SUCCESS) {
        return false;
    }
    result.graphicsFamily = indices.graphics.value();
    result.presentFamily = indices.present.value();
    vkGetDeviceQueue(result.device, result.graphicsFamily, 0, &result.graphicsQueue);
    vkGetDeviceQueue(result.device, result.presentFamily, 0, &result.presentQueue);
    return true;
}

void cleanupPartialVulkan(VulkanInitResult& r) {
    if (r.device != VK_NULL_HANDLE) {
        vkDestroyDevice(r.device, nullptr);
    }
    if (r.surface != VK_NULL_HANDLE && r.instance != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(r.instance, r.surface, nullptr);
    }
    if (r.instance != VK_NULL_HANDLE) {
        vkDestroyInstance(r.instance, nullptr);
    }
    r = VulkanInitResult{};
}

bool initializeSDL() {
    VideoOptions opts{640,480,false,1};
    return svga_init(&opts);
}

VulkanInitResult initializeVulkan(HWND hwnd) {
    VulkanInitResult result;
    uint32_t instanceVersion;
    if (vkEnumerateInstanceVersion(&instanceVersion) != VK_SUCCESS) {
        result.errorMessage = "Vulkan not supported on this system";
        return result;
    }
    if (instanceVersion < VK_MAKE_VERSION(1,1,0)) {
        result.errorMessage = "Vulkan 1.1+ required";
        return result;
    }

    std::vector<const char*> extensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
#ifdef _DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    if (!checkExtensionSupport(extensions)) {
        result.errorMessage = "Required Vulkan extensions not available";
        return result;
    }

    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "Fallout1-CE";
    app.applicationVersion = VK_MAKE_VERSION(1,0,0);
    app.pEngineName = "Fallout1-CE";
    app.engineVersion = VK_MAKE_VERSION(1,0,0);
    app.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo create{};
    create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create.pApplicationInfo = &app;
    create.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    create.ppEnabledExtensionNames = extensions.data();
#ifdef _DEBUG
    const char* validation = "VK_LAYER_KHRONOS_validation";
    if (checkValidationLayerSupport(validation)) {
        create.enabledLayerCount = 1;
        create.ppEnabledLayerNames = &validation;
    }
#endif
    if (vkCreateInstance(&create, nullptr, &result.instance) != VK_SUCCESS) {
        result.errorMessage = "Failed to create Vulkan instance";
        return result;
    }

    VkWin32SurfaceCreateInfoKHR surf{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
    surf.hwnd = hwnd;
    surf.hinstance = GetModuleHandle(nullptr);
    if (vkCreateWin32SurfaceKHR(result.instance, &surf, nullptr, &result.surface) != VK_SUCCESS) {
        result.errorMessage = "Failed to create window surface";
        return result;
    }
    if (!selectBestPhysicalDevice(result)) {
        result.errorMessage = "No suitable Vulkan device found";
        return result;
    }
    if (!createLogicalDevice(result)) {
        result.errorMessage = "Failed to create logical device";
        return result;
    }
    result.success = true;
    return result;
}

RenderBackend g_renderBackend = RenderBackend::SDL;
VulkanInitResult g_vulkanContext;

bool initializeRenderer() {
    VulkanInitResult vulkanResult = initializeVulkan(getHWND(gSdlWindow));
    if (vulkanResult.success) {
        g_vulkanContext = vulkanResult;
        g_renderBackend = RenderBackend::VULKAN_BATCH;
        debug_printf("Vulkan renderer initialized successfully\n");
        return true;
    }
    debug_printf("Vulkan initialization failed: %s\n", vulkanResult.errorMessage.c_str());
    VulkanErrorReporter::generateDiagnosticReport(vulkanResult.errorMessage);
    debug_printf("Falling back to SDL renderer\n");
    cleanupPartialVulkan(vulkanResult);
    if (initializeSDL()) {
        g_renderBackend = RenderBackend::SDL;
        debug_printf("SDL renderer initialized as fallback\n");
        return true;
    }
    debug_printf("Both Vulkan and SDL renderers failed to initialize\n");
    return false;
}

} // anonymous namespace

class VulkanBackendRenderer : public IRenderer {
public:
    bool initialize(SDL_Window* window) override {
        return initializeRenderer();
    }
    void render() override {}
    void resize(int, int) override {}
    void shutdown() override {
        cleanupPartialVulkan(g_vulkanContext);
    }
};

std::unique_ptr<IRenderer> RendererFactory_create(SDL_Window* window) {
    auto vulkanRenderer = std::make_unique<VulkanBackendRenderer>();
    if (vulkanRenderer->initialize(window)) {
        saveRendererChoice("VULKAN");
        return vulkanRenderer;
    }
    auto sdlRenderer = std::make_unique<SDLRenderer>();
    if (sdlRenderer->initialize(window)) {
        saveRendererChoice("SDL");
        return sdlRenderer;
    }
    return nullptr;
}

} // namespace fallout
