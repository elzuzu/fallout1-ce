#pragma once
#include <vulkan/vulkan.h>

namespace fallout {
struct GpuLimits {
    bool bufferDeviceAddress = true;
    bool ycbcrConversion = true;
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
};

inline GpuLimits query_platform_limits(VkPhysicalDevice phys)
{
    GpuLimits g{};
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(phys, &props);
    if (strstr(props.deviceName, "MoltenVK")) {
        g.bufferDeviceAddress = false;
        g.ycbcrConversion = false;
        g.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    }
    return g;
}
}
