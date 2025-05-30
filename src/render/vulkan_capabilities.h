#ifndef FALLOUT_RENDER_VULKAN_CAPABILITIES_H_
#define FALLOUT_RENDER_VULKAN_CAPABILITIES_H_

#include <vulkan/vulkan.h>

namespace fallout {

class VulkanCapabilities {
public:
    bool init(VkPhysicalDevice physicalDevice);

    bool hasGeometryShader = false;
    bool hasComputeShader = false;
    uint32_t maxTextureSize = 0;
};

extern VulkanCapabilities gVulkanCaps;

} // namespace fallout

#endif /* FALLOUT_RENDER_VULKAN_CAPABILITIES_H_ */
