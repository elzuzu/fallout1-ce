#pragma once

#include <SDL.h>
#include <vector>
#include <vulkan/vulkan.h>

namespace fallout {

class VulkanSwapchain {
public:
    VkResult create(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface,
        SDL_Window* window,
        VkRenderPass renderPass);
    VkResult recreate(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkSurfaceKHR surface,
        SDL_Window* window);
    void destroy(VkDevice device);

    VkSwapchainKHR get() const { return swapchain_; }
    const std::vector<VkImage>& getImages() const { return swapchainImages_; }
    const std::vector<VkImageView>& getImageViews() const { return swapchainImageViews_; }
    const std::vector<VkFramebuffer>& getFramebuffers() const { return swapchainFramebuffers_; }
    VkFormat getImageFormat() const { return swapchainImageFormat_; }
    VkExtent2D getExtent() const { return swapchainExtent_; }

private:
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    std::vector<VkFramebuffer> swapchainFramebuffers_;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent_ {};
};

} // namespace fallout
