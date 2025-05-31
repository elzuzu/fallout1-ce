#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <algorithm>

namespace fallout {

struct SwapchainData {
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
    VkRenderPass renderPass = VK_NULL_HANDLE;
    bool needsRecreation = false;
};

enum class AcquireResult {
    SUCCESS,
    OUT_OF_DATE,
    SUBOPTIMAL,
    ERROR
};

class VulkanSwapchain {
public:
    VulkanSwapchain() = default;

    void init(VkDevice dev,
              VkPhysicalDevice phys,
              VkSurfaceKHR surf,
              uint32_t graphicsFam,
              uint32_t presentFam,
              VkRenderPass rp)
    {
        device = dev;
        physicalDevice = phys;
        surface = surf;
        graphicsFamily = graphicsFam;
        presentFamily = presentFam;
        current.renderPass = rp;
    }

    bool create(uint32_t width, uint32_t height)
    {
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

        VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat();
        VkPresentModeKHR presentMode = choosePresentMode();
        VkExtent2D extent = chooseExtent(capabilities, width, height);

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0)
            imageCount = std::min(imageCount, capabilities.maxImageCount);

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = current.swapchain;

        uint32_t queueFamilyIndices[] = {graphicsFamily, presentFamily};
        if (graphicsFamily != presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        VkSwapchainKHR newSwapchain;
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &newSwapchain) != VK_SUCCESS)
            return false;

        previous = current;
        current.swapchain = newSwapchain;
        current.format = surfaceFormat.format;
        current.extent = extent;
        current.needsRecreation = false;
        current.renderPass = current.renderPass == VK_NULL_HANDLE ? createInfo.oldSwapchain : current.renderPass;

        if (!createImageViews() || !createFramebuffers())
            return false;

        return true;
    }

    AcquireResult acquireNextImage(uint32_t* imageIndex, VkSemaphore semaphore)
    {
        VkResult result = vkAcquireNextImageKHR(device, current.swapchain, UINT64_MAX,
                                                semaphore, VK_NULL_HANDLE, imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            current.needsRecreation = true;
            return AcquireResult::OUT_OF_DATE;
        } else if (result == VK_SUBOPTIMAL_KHR) {
            return AcquireResult::SUBOPTIMAL;
        } else if (result == VK_SUCCESS) {
            return AcquireResult::SUCCESS;
        }
        return AcquireResult::ERROR;
    }

    bool presentImage(uint32_t imageIndex, VkSemaphore waitSemaphore, VkQueue presentQueue)
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &current.swapchain;
        presentInfo.pImageIndices = &imageIndex;

        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            current.needsRecreation = true;
            return false;
        }
        return result == VK_SUCCESS;
    }

    bool recreateIfNeeded(uint32_t newWidth, uint32_t newHeight)
    {
        if (!current.needsRecreation &&
            current.extent.width == newWidth &&
            current.extent.height == newHeight)
            return true;

        vkDeviceWaitIdle(device);
        cleanupPrevious();
        return create(newWidth, newHeight);
    }

    const SwapchainData& getData() const { return current; }

    void cleanupPrevious()
    {
        for (VkFramebuffer fb : previous.framebuffers)
            vkDestroyFramebuffer(device, fb, nullptr);
        previous.framebuffers.clear();
        for (VkImageView view : previous.imageViews)
            vkDestroyImageView(device, view, nullptr);
        previous.imageViews.clear();
        if (previous.swapchain != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(device, previous.swapchain, nullptr);
        previous = {};
    }

private:
    VkSurfaceFormatKHR chooseSurfaceFormat()
    {
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
        for (const auto& fmt : formats) {
            if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB &&
                fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return fmt;
        }
        return formats[0];
    }

    VkPresentModeKHR choosePresentMode()
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr);
        std::vector<VkPresentModeKHR> modes(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, modes.data());
        for (const auto& mode : modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return mode;
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t w, uint32_t h)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
            return capabilities.currentExtent;
        VkExtent2D extent{w, h};
        extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return extent;
    }

    bool createImageViews()
    {
        uint32_t count = 0;
        vkGetSwapchainImagesKHR(device, current.swapchain, &count, nullptr);
        current.images.resize(count);
        vkGetSwapchainImagesKHR(device, current.swapchain, &count, current.images.data());

        current.imageViews.resize(count);
        for (uint32_t i = 0; i < count; ++i) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = current.images[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = current.format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;
            if (vkCreateImageView(device, &viewInfo, nullptr, &current.imageViews[i]) != VK_SUCCESS)
                return false;
        }
        return true;
    }

    bool createFramebuffers()
    {
        if (current.renderPass == VK_NULL_HANDLE)
            return false;
        current.framebuffers.resize(current.imageViews.size());
        for (size_t i = 0; i < current.imageViews.size(); ++i) {
            VkImageView attachments[] = {current.imageViews[i]};
            VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            fbInfo.renderPass = current.renderPass;
            fbInfo.attachmentCount = 1;
            fbInfo.pAttachments = attachments;
            fbInfo.width = current.extent.width;
            fbInfo.height = current.extent.height;
            fbInfo.layers = 1;
            if (vkCreateFramebuffer(device, &fbInfo, nullptr, &current.framebuffers[i]) != VK_SUCCESS)
                return false;
        }
        return true;
    }

    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    uint32_t graphicsFamily = 0;
    uint32_t presentFamily = 0;
    SwapchainData current{};
    SwapchainData previous{};
};

} // namespace fallout

