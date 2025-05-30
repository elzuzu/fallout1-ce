#include "render/vulkan_capabilities.h"

namespace fallout {

bool VulkanCapabilities::init(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceFeatures features{};
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);
    hasGeometryShader = features.geometryShader == VK_TRUE;
    hasComputeShader = features.computeShader == VK_TRUE;

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    maxTextureSize = props.limits.maxImageDimension2D;
    return true;
}

VulkanCapabilities gVulkanCaps;

} // namespace fallout
