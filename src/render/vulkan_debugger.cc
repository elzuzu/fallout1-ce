#include "render/vulkan_debugger.h"
#include "plib/gnw/debug.h"

namespace fallout {

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    debug_printf("Vulkan: %s\n", pCallbackData->pMessage);
    FILE* log = reinterpret_cast<FILE*>(pUserData);
    if (log) {
        fprintf(log, "Vulkan: %s\n", pCallbackData->pMessage);
        fflush(log);
    }
    return VK_FALSE;
}

VulkanDebugger gVulkanDebugger;

bool VulkanDebugger::init(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
    m_instance = instance;
    m_device = device;
    m_physicalDevice = physicalDevice;

    m_logFile = fopen("vulkan_debug.log", "wt");

    PFN_vkCreateDebugUtilsMessengerEXT createFunc =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (createFunc) {
        VkDebugUtilsMessengerCreateInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback = debug_callback;
        info.pUserData = m_logFile;
        createFunc(instance, &info, nullptr, &m_messenger);
    }

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
    m_timestampPeriod = props.limits.timestampPeriod;

    VkQueryPoolCreateInfo qp{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    qp.queryType = VK_QUERY_TYPE_TIMESTAMP;
    qp.queryCount = 64; // enough for several frames
    vkCreateQueryPool(m_device, &qp, nullptr, &m_queryPool);

    return true;
}

void VulkanDebugger::destroy()
{
    if (m_device != VK_NULL_HANDLE) {
        if (m_queryPool != VK_NULL_HANDLE)
            vkDestroyQueryPool(m_device, m_queryPool, nullptr);
    }
    if (m_messenger != VK_NULL_HANDLE) {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyFunc =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroyFunc)
            destroyFunc(m_instance, m_messenger, nullptr);
    }
    if (m_logFile) {
        fclose(m_logFile);
        m_logFile = nullptr;
    }
    m_instance = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_messenger = VK_NULL_HANDLE;
    m_queryPool = VK_NULL_HANDLE;
}

void VulkanDebugger::begin_frame(uint32_t frameIndex, VkCommandBuffer cmd)
{
    if (m_queryPool == VK_NULL_HANDLE)
        return;
    uint32_t idx = frameIndex * 2;
    vkCmdResetQueryPool(cmd, m_queryPool, idx, 2);
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_queryPool, idx);
}

void VulkanDebugger::end_frame(uint32_t frameIndex, VkCommandBuffer cmd)
{
    if (m_queryPool == VK_NULL_HANDLE)
        return;
    uint32_t idx = frameIndex * 2 + 1;
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPool, idx);
}

void VulkanDebugger::resolve_frame(uint32_t frameIndex)
{
    if (m_queryPool == VK_NULL_HANDLE)
        return;
    uint64_t timestamps[2]{};
    uint32_t idx = frameIndex * 2;
    if (vkGetQueryPoolResults(m_device, m_queryPool, idx, 2, sizeof(timestamps), timestamps,
            sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT) == VK_SUCCESS) {
        double ms = (timestamps[1] - timestamps[0]) * m_timestampPeriod / 1e6;
        debug_printf("GPU Frame %u: %.3f ms\n", frameIndex, ms);
        if (m_logFile) {
            fprintf(m_logFile, "Frame %u: %.3f ms\n", frameIndex, ms);
            fflush(m_logFile);
        }
        m_lastFrameMs = ms;
    }
}

} // namespace fallout
