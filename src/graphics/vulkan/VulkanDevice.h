#ifndef FALLOUT_GRAPHICS_VULKAN_VULKAN_DEVICE_H_
#define FALLOUT_GRAPHICS_VULKAN_VULKAN_DEVICE_H_

#include <vector>
#include <vulkan/vulkan.h>

namespace fallout {

struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;
    bool isComplete() const { return graphicsFamily >= 0 && presentFamily >= 0; }
};

struct SwapChainSupportDetails {
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanDevice {
public:
    bool selectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice_; }
    bool createLogicalDevice(VkSurfaceKHR surface, VkDevice& device,
        VkQueue& graphicsQueue,
        uint32_t& graphicsQueueFamily);

private:
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
};

} // namespace fallout

#endif // FALLOUT_GRAPHICS_VULKAN_VULKAN_DEVICE_H_
