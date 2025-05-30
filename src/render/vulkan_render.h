#ifndef FALLOUT_RENDER_VULKAN_RENDER_H_
#define FALLOUT_RENDER_VULKAN_RENDER_H_

#include "plib/gnw/svga_types.h"
#include <SDL.h>
#include <vector>
#include <vulkan/vulkan.h>

namespace fallout {

class VulkanRenderer {
public:
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D swapchainExtent {};
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    VkImage internalImage = VK_NULL_HANDLE;
    VkDeviceMemory internalImageMemory = VK_NULL_HANDLE;
    VkImageView internalImageView = VK_NULL_HANDLE;
    VkExtent2D internalExtent {};
    SDL_Surface* frameSurface = nullptr;
    SDL_Surface* frameTextureSurface = nullptr;
};

// Initializes the Vulkan renderer. Returns true on success.
bool vulkan_render_init(VideoOptions* options);

// Cleans up Vulkan renderer resources.
void vulkan_render_exit();

// Handles window size change for Vulkan backend.
void vulkan_render_handle_window_size_changed();

// Presents the current frame using Vulkan.
void vulkan_render_present();

// Accessors for the software surfaces used by the engine when rendering with
// the Vulkan backend.
SDL_Surface* vulkan_render_get_surface();
SDL_Surface* vulkan_render_get_texture_surface();

// Returns true if Vulkan support is available on the system.
bool vulkan_is_available();

} // namespace fallout

#endif /* FALLOUT_RENDER_VULKAN_RENDER_H_ */
