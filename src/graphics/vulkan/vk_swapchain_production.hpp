#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <cstdio>
#include <cstdarg>
#include <algorithm>
#include <string>

namespace fallout {

inline void logError(const char* fmt, ...) {
    va_list args; va_start(args, fmt); vfprintf(stderr, fmt, args); va_end(args); fprintf(stderr, "\n");
}
inline void logInfo(const char* fmt, ...) {
    va_list args; va_start(args, fmt); vfprintf(stdout, fmt, args); va_end(args); fprintf(stdout, "\n");
}
inline void logWarning(const char* fmt, ...) {
    va_list args; va_start(args, fmt); vfprintf(stdout, fmt, args); va_end(args); fprintf(stdout, "\n");
}

inline std::string translateVulkanError(VkResult res)
{
    switch (res) {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_NOT_READY: return "VK_NOT_READY";
    case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_EVENT_SET: return "VK_EVENT_SET";
    case VK_EVENT_RESET: return "VK_EVENT_RESET";
    case VK_INCOMPLETE: return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
    default: return "UNKNOWN";
    }
}

inline const char* presentModeToString(VkPresentModeKHR mode)
{
    switch (mode) {
    case VK_PRESENT_MODE_FIFO_KHR: return "FIFO";
    case VK_PRESENT_MODE_MAILBOX_KHR: return "MAILBOX";
    case VK_PRESENT_MODE_IMMEDIATE_KHR: return "IMMEDIATE";
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "FIFO_RELAXED";
    default: return "UNKNOWN";
    }
}

struct SwapchainData {
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkExtent2D extent{};
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t imageCount = 0;
    bool needsRecreation = false;
};

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanSwapchain {
private:
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;

    SwapchainData current{};
    SwapchainData previous{};

    uint32_t graphicsFamily = 0;
    uint32_t presentFamily = 0;

    bool enableVsync = true;
    bool preferHighRefreshRate = false;

public:
    bool initialize(VkDevice dev, VkPhysicalDevice physDev, VkSurfaceKHR surf,
                   VkRenderPass rp, uint32_t gfxFamily, uint32_t presFamily)
    {
        device = dev;
        physicalDevice = physDev;
        surface = surf;
        renderPass = rp;
        graphicsFamily = gfxFamily;
        presentFamily = presFamily;
        return true;
    }

    bool create(uint32_t width, uint32_t height, bool forceRecreate = false)
    {
        if (!forceRecreate && current.swapchain != VK_NULL_HANDLE &&
            current.extent.width == width && current.extent.height == height &&
            !current.needsRecreation) {
            return true;
        }

        vkDeviceWaitIdle(device);

        SwapchainSupportDetails swapchainSupport = querySwapchainSupport();
        if (swapchainSupport.formats.empty() || swapchainSupport.presentModes.empty()) {
            logError("Swapchain support insufficient");
            return false;
        }

        VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapchainSupport.formats);
        VkPresentModeKHR presentMode = choosePresentMode(swapchainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities, width, height);

        uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
        if (swapchainSupport.capabilities.maxImageCount > 0) {
            imageCount = std::min(imageCount, swapchainSupport.capabilities.maxImageCount);
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = {graphicsFamily, presentFamily};
        if (graphicsFamily != presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = current.swapchain;

        VkSwapchainKHR newSwapchain;
        VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &newSwapchain);
        if (result != VK_SUCCESS) {
            logError("Failed to create swapchain: %s", translateVulkanError(result).c_str());
            return false;
        }

        cleanupPrevious();
        previous = current;

        current = {};
        current.swapchain = newSwapchain;
        current.format = surfaceFormat.format;
        current.colorSpace = surfaceFormat.colorSpace;
        current.extent = extent;
        current.presentMode = presentMode;
        current.needsRecreation = false;

        if (!retrieveSwapchainImages() || !createImageViews() || !createFramebuffers()) {
            logError("Failed to setup swapchain resources");
            return false;
        }

        logInfo("Swapchain created successfully: %dx%d, %u images, %s", extent.width,
                extent.height, current.imageCount, presentModeToString(presentMode));

        return true;
    }

    VkSwapchainKHR getSwapchain() const { return current.swapchain; }
    void markForRecreation() { current.needsRecreation = true; }
    bool needsRecreation() const { return current.needsRecreation; }

private:
    SwapchainSupportDetails querySwapchainSupport()
    {
        SwapchainSupportDetails details{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        if (formatCount > 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
        if (presentModeCount > 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
        }
        return details;
    }

    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& format : availableFormats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        for (const auto& format : availableFormats) {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        if (!enableVsync) {
            for (const auto& mode : availablePresentModes) {
                if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                    return mode;
                }
            }
        }
        if (preferHighRefreshRate) {
            for (const auto& mode : availablePresentModes) {
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return mode;
                }
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
    {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }
        VkExtent2D actualExtent = {width, height};
        actualExtent.width = std::clamp(actualExtent.width,
                                       capabilities.minImageExtent.width,
                                       capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height,
                                        capabilities.minImageExtent.height,
                                        capabilities.maxImageExtent.height);
        return actualExtent;
    }

    bool retrieveSwapchainImages()
    {
        uint32_t count = 0;
        vkGetSwapchainImagesKHR(device, current.swapchain, &count, nullptr);
        current.images.resize(count);
        vkGetSwapchainImagesKHR(device, current.swapchain, &count, current.images.data());
        current.imageCount = count;
        return true;
    }

    bool createImageViews()
    {
        current.imageViews.resize(current.images.size());
        for (size_t i = 0; i < current.images.size(); ++i) {
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
        if (renderPass == VK_NULL_HANDLE)
            return false;
        current.framebuffers.resize(current.imageViews.size());
        for (size_t i = 0; i < current.imageViews.size(); ++i) {
            VkImageView attachments[] = {current.imageViews[i]};
            VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            fbInfo.renderPass = renderPass;
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
        previous.swapchain = VK_NULL_HANDLE;
    }
};

enum class AcquireResult {
    SUCCESS,
    OUT_OF_DATE,
    SUBOPTIMAL,
    ERROR,
    TIMEOUT
};

class SwapchainFrameManager {
private:
    VulkanSwapchain* swapchain = nullptr;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

public:
    bool initialize(VkDevice device, VulkanSwapchain* sc)
    {
        swapchain = sc;
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                logError("Failed to create synchronization objects");
                return false;
            }
        }
        return true;
    }

    AcquireResult acquireNextImage(VkDevice device, uint32_t* imageIndex)
    {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        VkResult result = vkAcquireNextImageKHR(device, swapchain->getSwapchain(), UINT64_MAX,
                                               imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return AcquireResult::OUT_OF_DATE;
        } else if (result == VK_SUBOPTIMAL_KHR) {
            return AcquireResult::SUBOPTIMAL;
        } else if (result == VK_TIMEOUT || result == VK_NOT_READY) {
            return AcquireResult::TIMEOUT;
        } else if (result != VK_SUCCESS) {
            logError("Failed to acquire swapchain image: %s", translateVulkanError(result).c_str());
            return AcquireResult::ERROR;
        }
        vkResetFences(device, 1, &inFlightFences[currentFrame]);
        return AcquireResult::SUCCESS;
    }

    bool presentImage(VkDevice device, VkQueue presentQueue, uint32_t imageIndex)
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
        VkSwapchainKHR sc[] = {swapchain->getSwapchain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = sc;
        presentInfo.pImageIndices = &imageIndex;
        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            swapchain->markForRecreation();
            return false;
        } else if (result != VK_SUCCESS) {
            logError("Failed to present swapchain image: %s", translateVulkanError(result).c_str());
            return false;
        }
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        return true;
    }

    VkSemaphore getCurrentImageAvailableSemaphore() const { return imageAvailableSemaphores[currentFrame]; }
    VkSemaphore getCurrentRenderFinishedSemaphore() const { return renderFinishedSemaphores[currentFrame]; }
    VkFence getCurrentInFlightFence() const { return inFlightFences[currentFrame]; }
};

class FalloutVulkanRenderer {
private:
    VulkanSwapchain swapchain;
    SwapchainFrameManager frameManager;
    uint32_t currentWindowWidth = 0;
    uint32_t currentWindowHeight = 0;
    bool windowResized = false;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

public:
    void setup(VkDevice dev, VkQueue gfx, VkQueue present, VulkanSwapchain sc)
    {
        device = dev;
        graphicsQueue = gfx;
        presentQueue = present;
        swapchain = sc;
    }

    void handleWindowResize(uint32_t newWidth, uint32_t newHeight)
    {
        if (newWidth != currentWindowWidth || newHeight != currentWindowHeight) {
            currentWindowWidth = newWidth;
            currentWindowHeight = newHeight;
            windowResized = true;
            logInfo("Window resized to %dx%d, will recreate swapchain", newWidth, newHeight);
        }
    }

    void renderFrame()
    {
        if (windowResized || swapchain.needsRecreation()) {
            if (currentWindowWidth == 0 || currentWindowHeight == 0) {
                return;
            }
            if (!recreateSwapchain()) {
                logError("Failed to recreate swapchain");
                fallbackToSDL();
                return;
            }
            windowResized = false;
        }

        uint32_t imageIndex = 0;
        AcquireResult acquireResult = frameManager.acquireNextImage(device, &imageIndex);
        if (acquireResult == AcquireResult::OUT_OF_DATE) {
            if (!recreateSwapchain()) {
                fallbackToSDL();
                return;
            }
            return;
        }
        if (acquireResult == AcquireResult::TIMEOUT) {
            return;
        }
        if (acquireResult == AcquireResult::ERROR) {
            logError("Critical swapchain error, falling back to SDL");
            fallbackToSDL();
            return;
        }

        VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
        recordRenderCommands(commandBuffer, imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = {frameManager.getCurrentImageAvailableSemaphore()};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        VkSemaphore signalSemaphores[] = {frameManager.getCurrentRenderFinishedSemaphore()};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameManager.getCurrentInFlightFence());
        if (result != VK_SUCCESS) {
            logError("Failed to submit draw command buffer: %s", translateVulkanError(result).c_str());
            return;
        }

        if (!frameManager.presentImage(device, presentQueue, imageIndex)) {
        }
    }

private:
    bool recreateSwapchain()
    {
        return swapchain.create(currentWindowWidth, currentWindowHeight, true);
    }
    void fallbackToSDL()
    {
        logWarning("Falling back to SDL renderer due to Vulkan issues");
    }
    VkCommandBuffer getCurrentCommandBuffer();
    void recordRenderCommands(VkCommandBuffer, uint32_t);
};

} // namespace fallout

