#ifndef FALLOUT_GRAPHICS_VULKAN_VULKAN_INSTANCE_H_
#define FALLOUT_GRAPHICS_VULKAN_VULKAN_INSTANCE_H_

#include <SDL.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace fallout {

class VulkanInstance {
public:
    VulkanInstance() = default;
    ~VulkanInstance();

    bool initialize(SDL_Window* window, bool enableValidation);
    void shutdown();

    VkInstance get() const { return instance_; }

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;

    bool setup_debug_messenger(const VkInstanceCreateInfo& createInfo);
};

} // namespace fallout

#endif // FALLOUT_GRAPHICS_VULKAN_VULKAN_INSTANCE_H_
