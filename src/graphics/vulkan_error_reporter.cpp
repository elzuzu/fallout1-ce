#include "graphics/vulkan_error_reporter.hpp"
#include "plib/gnw/debug.h"
#include <SDL.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/utsname.h>
#endif

using namespace fallout;

std::string VulkanErrorReporter::translateVulkanError(VkResult result) {
    switch (result) {
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "Out of host memory (RAM)";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "Out of device memory (VRAM)";
        case VK_ERROR_INITIALIZATION_FAILED: return "Vulkan initialization failed";
        case VK_ERROR_DEVICE_LOST: return "Graphics device lost (driver crash/reset)";
        case VK_ERROR_MEMORY_MAP_FAILED: return "Memory mapping failed";
        case VK_ERROR_LAYER_NOT_PRESENT: return "Validation layer not available";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "Required extension not supported";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "Required feature not supported";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "Incompatible Vulkan driver";
        case VK_ERROR_TOO_MANY_OBJECTS: return "Too many Vulkan objects created";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "Image format not supported";
        case VK_ERROR_SURFACE_LOST_KHR: return "Window surface lost";
        case VK_ERROR_OUT_OF_DATE_KHR: return "Swapchain out of date (window resized)";
        case VK_SUBOPTIMAL_KHR: return "Swapchain suboptimal";
        default: return "Unknown Vulkan error (" + std::to_string(result) + ")";
    }
}

void VulkanErrorReporter::generateDiagnosticReport(const std::string& error) {
    std::ofstream report("fallout_vulkan_diagnostic.txt");
    report << "Fallout1-CE Vulkan Diagnostic Report\n";
    report << "=====================================\n\n";
    report << "Error: " << error << "\n\n";

    report << "System Information:\n";
    report << "- OS: " << getOSVersion() << "\n";
    report << "- CPU: " << getCPUInfo() << "\n";

    report << "\nVulkan Information:\n";
    if (isVulkanRuntimeInstalled()) {
        report << "- Runtime: Installed\n";
        report << "- Version: " << getVulkanVersion() << "\n";
        listAvailableDevices(report);
    } else {
        report << "- Runtime: NOT INSTALLED\n";
        report << "- Recommendation: Install latest graphics drivers\n";
    }
    report.close();
    logInfo("Diagnostic report saved to fallout_vulkan_diagnostic.txt");
}

std::string VulkanErrorReporter::getOSVersion() {
#ifdef _WIN32
    OSVERSIONINFOEX info{0};
    info.dwOSVersionInfoSize = sizeof(info);
    if (GetVersionEx((OSVERSIONINFO*)&info)) {
        return std::string("Windows ") + std::to_string(info.dwMajorVersion) + "." + std::to_string(info.dwMinorVersion);
    }
    return "Windows";
#else
    struct utsname buf;
    if (uname(&buf) == 0) {
        return std::string(buf.sysname) + " " + buf.release;
    }
    return "Unknown";
#endif
}

std::string VulkanErrorReporter::getCPUInfo() {
    int cores = SDL_GetCPUCount();
    return std::to_string(cores) + " cores";
}

bool VulkanErrorReporter::isVulkanRuntimeInstalled() {
    uint32_t version = 0;
    return vkEnumerateInstanceVersion(&version) == VK_SUCCESS;
}

std::string VulkanErrorReporter::getVulkanVersion() {
    uint32_t version = 0;
    if (vkEnumerateInstanceVersion(&version) != VK_SUCCESS) return "Unknown";
    return std::to_string(VK_VERSION_MAJOR(version)) + "." + std::to_string(VK_VERSION_MINOR(version)) + "." + std::to_string(VK_VERSION_PATCH(version));
}

void VulkanErrorReporter::listAvailableDevices(std::ofstream& report) {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app;
    VkInstance tempInstance = VK_NULL_HANDLE;
    if (vkCreateInstance(&ci, nullptr, &tempInstance) != VK_SUCCESS) {
        report << "- Unable to create temporary instance for device listing\n";
        return;
    }
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(tempInstance, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(tempInstance, &count, devices.data());
    report << "- Devices (" << count << "):\n";
    for (auto dev : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);
        report << "  - " << props.deviceName << "\n";
    }
    vkDestroyInstance(tempInstance, nullptr);
}

void VulkanErrorReporter::logInfo(const std::string& msg) {
    debug_printf("%s\n", msg.c_str());
}
