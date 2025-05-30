#ifndef FALLOUT_RENDER_VULKAN_DEBUGGER_H_
#define FALLOUT_RENDER_VULKAN_DEBUGGER_H_

#include <vulkan/vulkan.h>
#include <cstdio>

namespace fallout {

class VulkanDebugger {
public:
    bool init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
    void destroy();

    void begin_frame(uint32_t frameIndex, VkCommandBuffer cmd);
    void end_frame(uint32_t frameIndex, VkCommandBuffer cmd);
    void resolve_frame(uint32_t frameIndex);

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_messenger = VK_NULL_HANDLE;
    VkQueryPool m_queryPool = VK_NULL_HANDLE;
    double m_timestampPeriod = 1.0;
    FILE* m_logFile = nullptr;
};

extern VulkanDebugger gVulkanDebugger;

} // namespace fallout

#endif /* FALLOUT_RENDER_VULKAN_DEBUGGER_H_ */
