#include "render/vulkan_render.h"
#include "plib/gnw/svga.h"

#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <vector>

namespace fallout {
namespace {
    struct VulkanContext {
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
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        VkSemaphore renderFinished = VK_NULL_HANDLE;
        VkFence inFlight = VK_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;
    } gVulkan;

    static bool create_swapchain(uint32_t width, uint32_t height)
    {
        VkSurfaceCapabilitiesKHR surfaceCaps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gVulkan.physicalDevice, gVulkan.surface, &surfaceCaps);

        gVulkan.swapchainExtent = surfaceCaps.currentExtent;
        if (gVulkan.swapchainExtent.width == UINT32_MAX) {
            gVulkan.swapchainExtent.width = width;
            gVulkan.swapchainExtent.height = height;
        }

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(gVulkan.physicalDevice, gVulkan.surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(gVulkan.physicalDevice, gVulkan.surface, &formatCount, formats.data());
        VkSurfaceFormatKHR surfaceFormat = formats.empty() ? VkSurfaceFormatKHR { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR } : formats[0];
        gVulkan.swapchainImageFormat = surfaceFormat.format;

        VkSwapchainCreateInfoKHR swapInfo { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapInfo.surface = gVulkan.surface;
        swapInfo.minImageCount = surfaceCaps.minImageCount + 1;
        if (surfaceCaps.maxImageCount > 0 && swapInfo.minImageCount > surfaceCaps.maxImageCount)
            swapInfo.minImageCount = surfaceCaps.maxImageCount;
        swapInfo.imageFormat = surfaceFormat.format;
        swapInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapInfo.imageExtent = gVulkan.swapchainExtent;
        swapInfo.imageArrayLayers = 1;
        swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapInfo.preTransform = surfaceCaps.currentTransform;
        swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapInfo.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(gVulkan.device, &swapInfo, nullptr, &gVulkan.swapchain) != VK_SUCCESS)
            return false;

        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(gVulkan.device, gVulkan.swapchain, &imageCount, nullptr);
        gVulkan.swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(gVulkan.device, gVulkan.swapchain, &imageCount, gVulkan.swapchainImages.data());

        gVulkan.swapchainImageViews.resize(imageCount);
        for (uint32_t i = 0; i < imageCount; i++) {
            VkImageViewCreateInfo viewInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            viewInfo.image = gVulkan.swapchainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = gVulkan.swapchainImageFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.layerCount = 1;
            if (vkCreateImageView(gVulkan.device, &viewInfo, nullptr, &gVulkan.swapchainImageViews[i]) != VK_SUCCESS)
                return false;
        }

        return true;
    }

    static void destroy_swapchain()
    {
        for (VkImageView view : gVulkan.swapchainImageViews)
            vkDestroyImageView(gVulkan.device, view, nullptr);
        gVulkan.swapchainImageViews.clear();

        if (gVulkan.swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(gVulkan.device, gVulkan.swapchain, nullptr);
            gVulkan.swapchain = VK_NULL_HANDLE;
        }
    }
} // namespace

bool vulkan_render_init(VideoOptions* options)
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
        return false;

    Uint32 flags = SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI;
    if (options->fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;

    gSdlWindow = SDL_CreateWindow(GNW95_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        options->width * options->scale, options->height * options->scale, flags);
    if (gSdlWindow == nullptr)
        return false;

    gVulkan.width = options->width * options->scale;
    gVulkan.height = options->height * options->scale;

    VkApplicationInfo appInfo { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName = "FalloutCE";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "FalloutCE";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    unsigned int extCount = 0;
    SDL_Vulkan_GetInstanceExtensions(gSdlWindow, &extCount, nullptr);
    std::vector<const char*> extensions(extCount);
    SDL_Vulkan_GetInstanceExtensions(gSdlWindow, &extCount, extensions.data());

    VkInstanceCreateInfo instInfo { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = extCount;
    instInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&instInfo, nullptr, &gVulkan.instance) != VK_SUCCESS)
        return false;

    if (!SDL_Vulkan_CreateSurface(gSdlWindow, gVulkan.instance, &gVulkan.surface))
        return false;

    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(gVulkan.instance, &gpuCount, nullptr);
    if (gpuCount == 0)
        return false;
    std::vector<VkPhysicalDevice> gpus(gpuCount);
    vkEnumeratePhysicalDevices(gVulkan.instance, &gpuCount, gpus.data());
    gVulkan.physicalDevice = gpus[0];

    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gVulkan.physicalDevice, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(gVulkan.physicalDevice, &familyCount, families.data());

    for (uint32_t i = 0; i < familyCount; i++) {
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(gVulkan.physicalDevice, i, gVulkan.surface, &presentSupport);
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
            gVulkan.graphicsQueueFamily = i;
            break;
        }
    }

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = gVulkan.graphicsQueueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo devInfo { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    devInfo.queueCreateInfoCount = 1;
    devInfo.pQueueCreateInfos = &queueInfo;
    devInfo.enabledExtensionCount = 1;
    devInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(gVulkan.physicalDevice, &devInfo, nullptr, &gVulkan.device) != VK_SUCCESS)
        return false;

    vkGetDeviceQueue(gVulkan.device, gVulkan.graphicsQueueFamily, 0, &gVulkan.graphicsQueue);

    if (!create_swapchain(gVulkan.width, gVulkan.height))
        return false;

    VkCommandPoolCreateInfo poolInfo { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.queueFamilyIndex = gVulkan.graphicsQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(gVulkan.device, &poolInfo, nullptr, &gVulkan.commandPool) != VK_SUCCESS)
        return false;

    VkCommandBufferAllocateInfo cmdInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    cmdInfo.commandPool = gVulkan.commandPool;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(gVulkan.device, &cmdInfo, &gVulkan.commandBuffer) != VK_SUCCESS)
        return false;

    VkSemaphoreCreateInfo semInfo { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(gVulkan.device, &semInfo, nullptr, &gVulkan.imageAvailable);
    vkCreateSemaphore(gVulkan.device, &semInfo, nullptr, &gVulkan.renderFinished);

    VkFenceCreateInfo fenceInfo { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(gVulkan.device, &fenceInfo, nullptr, &gVulkan.inFlight);

    return true;
}

void vulkan_render_exit()
{
    if (gVulkan.device == VK_NULL_HANDLE)
        return;

    vkDeviceWaitIdle(gVulkan.device);

    vkDestroyFence(gVulkan.device, gVulkan.inFlight, nullptr);
    vkDestroySemaphore(gVulkan.device, gVulkan.renderFinished, nullptr);
    vkDestroySemaphore(gVulkan.device, gVulkan.imageAvailable, nullptr);

    vkDestroyCommandPool(gVulkan.device, gVulkan.commandPool, nullptr);

    destroy_swapchain();

    if (gVulkan.surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(gVulkan.instance, gVulkan.surface, nullptr);
    if (gVulkan.device != VK_NULL_HANDLE)
        vkDestroyDevice(gVulkan.device, nullptr);
    if (gVulkan.instance != VK_NULL_HANDLE)
        vkDestroyInstance(gVulkan.instance, nullptr);

    if (gSdlWindow) {
        SDL_DestroyWindow(gSdlWindow);
        gSdlWindow = nullptr;
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void vulkan_render_handle_window_size_changed()
{
    if (gVulkan.device == VK_NULL_HANDLE)
        return;

    int w, h;
    SDL_Vulkan_GetDrawableSize(gSdlWindow, &w, &h);
    gVulkan.width = static_cast<uint32_t>(w);
    gVulkan.height = static_cast<uint32_t>(h);

    vkDeviceWaitIdle(gVulkan.device);

    destroy_swapchain();
    create_swapchain(gVulkan.width, gVulkan.height);
}

void vulkan_render_present()
{
    if (gVulkan.device == VK_NULL_HANDLE)
        return;

    vkWaitForFences(gVulkan.device, 1, &gVulkan.inFlight, VK_TRUE, UINT64_MAX);
    vkResetFences(gVulkan.device, 1, &gVulkan.inFlight);

    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(gVulkan.device, gVulkan.swapchain, UINT64_MAX, gVulkan.imageAvailable, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(gVulkan.commandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(gVulkan.commandBuffer, &beginInfo);

    VkClearValue clear {};
    clear.color.float32[0] = 0.f;
    clear.color.float32[1] = 0.f;
    clear.color.float32[2] = 0.f;
    clear.color.float32[3] = 1.f;

    VkRenderingAttachmentInfo colorAttachment { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    colorAttachment.imageView = gVulkan.swapchainImageViews[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clear;

    VkRenderingInfo renderingInfo { VK_STRUCTURE_TYPE_RENDERING_INFO };
    renderingInfo.renderArea.offset = { 0, 0 };
    renderingInfo.renderArea.extent = gVulkan.swapchainExtent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(gVulkan.commandBuffer, &renderingInfo);
    vkCmdEndRendering(gVulkan.commandBuffer);

    vkEndCommandBuffer(gVulkan.commandBuffer);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &gVulkan.imageAvailable;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &gVulkan.commandBuffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &gVulkan.renderFinished;

    vkQueueSubmit(gVulkan.graphicsQueue, 1, &submit, gVulkan.inFlight);

    VkPresentInfoKHR present { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &gVulkan.renderFinished;
    present.swapchainCount = 1;
    present.pSwapchains = &gVulkan.swapchain;
    present.pImageIndices = &imageIndex;

    vkQueuePresentKHR(gVulkan.graphicsQueue, &present);
}

SDL_Surface* vulkan_render_get_surface()
{
    return nullptr;
}

SDL_Surface* vulkan_render_get_texture_surface()
{
    return nullptr;
}

} // namespace fallout
