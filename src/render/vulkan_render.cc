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

        // Offscreen rendering resources for low resolution rendering
        VkImage internalImage = VK_NULL_HANDLE;
        VkDeviceMemory internalImageMemory = VK_NULL_HANDLE;
        VkImageView internalImageView = VK_NULL_HANDLE;
        VkExtent2D internalExtent{};

        // SDL surfaces used by the engine when running with the Vulkan
        // backend. These mirror the surfaces provided by the SDL renderer
        // but are not yet wired into the actual Vulkan rendering pipeline.
        SDL_Surface* frameSurface = nullptr;        // 8-bit indexed surface
        SDL_Surface* frameTextureSurface = nullptr; // 32-bit RGBA surface
    } gVulkan;

    static uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(gVulkan.physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        return 0;
    }

    static bool create_internal_image()
    {
        gVulkan.internalExtent.width = gVulkan.swapchainExtent.width / 2;
        gVulkan.internalExtent.height = gVulkan.swapchainExtent.height / 2;

        VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = gVulkan.internalExtent.width;
        imageInfo.extent.height = gVulkan.internalExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = gVulkan.swapchainImageFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(gVulkan.device, &imageInfo, nullptr, &gVulkan.internalImage) != VK_SUCCESS)
            return false;

        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(gVulkan.device, gVulkan.internalImage, &memReq);

        VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = find_memory_type(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(gVulkan.device, &allocInfo, nullptr, &gVulkan.internalImageMemory) != VK_SUCCESS)
            return false;

        vkBindImageMemory(gVulkan.device, gVulkan.internalImage, gVulkan.internalImageMemory, 0);

        VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.image = gVulkan.internalImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = gVulkan.swapchainImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(gVulkan.device, &viewInfo, nullptr, &gVulkan.internalImageView) != VK_SUCCESS)
            return false;

        return true;
    }

    static void destroy_internal_image()
    {
        if (gVulkan.internalImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(gVulkan.device, gVulkan.internalImageView, nullptr);
            gVulkan.internalImageView = VK_NULL_HANDLE;
        }
        if (gVulkan.internalImage != VK_NULL_HANDLE) {
            vkDestroyImage(gVulkan.device, gVulkan.internalImage, nullptr);
            gVulkan.internalImage = VK_NULL_HANDLE;
        }
        if (gVulkan.internalImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(gVulkan.device, gVulkan.internalImageMemory, nullptr);
            gVulkan.internalImageMemory = VK_NULL_HANDLE;
        }
    }

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

        if (!create_internal_image())
            return false;

        return true;
    }

    static void destroy_swapchain()
    {
        for (VkImageView view : gVulkan.swapchainImageViews)
            vkDestroyImageView(gVulkan.device, view, nullptr);
        gVulkan.swapchainImageViews.clear();

        destroy_internal_image();

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

    // Create software surfaces matching the game's logical resolution. These
    // are used by various subsystems to draw graphics before they are uploaded
    // to the Vulkan swapchain. For now they are CPU-only and the contents are
    // not yet copied to the Vulkan images.
    gVulkan.frameSurface = SDL_CreateRGBSurface(0,
        options->width,
        options->height,
        8,
        0,
        0,
        0,
        0);
    if (gVulkan.frameSurface == nullptr)
        return false;

    gVulkan.frameTextureSurface = SDL_CreateRGBSurfaceWithFormat(0,
        options->width,
        options->height,
        32,
        SDL_PIXELFORMAT_BGRA8888);
    if (gVulkan.frameTextureSurface == nullptr)
        return false;

    // Initialize grayscale palette like the SDL renderer does.
    if (gVulkan.frameSurface->format->palette != nullptr) {
        SDL_Color colors[256];
        for (int index = 0; index < 256; index++) {
            colors[index].r = index;
            colors[index].g = index;
            colors[index].b = index;
            colors[index].a = SDL_ALPHA_OPAQUE;
        }
        SDL_SetPaletteColors(gVulkan.frameSurface->format->palette, colors, 0, 256);
    }

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

    if (gVulkan.frameTextureSurface != nullptr) {
        SDL_FreeSurface(gVulkan.frameTextureSurface);
        gVulkan.frameTextureSurface = nullptr;
    }

    if (gVulkan.frameSurface != nullptr) {
        SDL_FreeSurface(gVulkan.frameSurface);
        gVulkan.frameSurface = nullptr;
    }

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

    // Render to the low resolution offscreen image
    VkClearValue clear{};
    clear.color.float32[0] = 0.f;
    clear.color.float32[1] = 0.f;
    clear.color.float32[2] = 0.f;
    clear.color.float32[3] = 1.f;

    VkRenderingAttachmentInfo offscreenAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    offscreenAttachment.imageView = gVulkan.internalImageView;
    offscreenAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    offscreenAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    offscreenAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    offscreenAttachment.clearValue = clear;

    VkRenderingInfo offscreenInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
    offscreenInfo.renderArea.offset = {0, 0};
    offscreenInfo.renderArea.extent = gVulkan.internalExtent;
    offscreenInfo.layerCount = 1;
    offscreenInfo.colorAttachmentCount = 1;
    offscreenInfo.pColorAttachments = &offscreenAttachment;

    vkCmdBeginRendering(gVulkan.commandBuffer, &offscreenInfo);
    vkCmdEndRendering(gVulkan.commandBuffer);

    // Transition offscreen image for blit
    VkImageMemoryBarrier offscreenBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    offscreenBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    offscreenBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    offscreenBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    offscreenBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    offscreenBarrier.image = gVulkan.internalImage;
    offscreenBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    offscreenBarrier.subresourceRange.levelCount = 1;
    offscreenBarrier.subresourceRange.layerCount = 1;
    offscreenBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    offscreenBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(gVulkan.commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &offscreenBarrier);

    // Transition swapchain image for blit destination
    VkImageMemoryBarrier swapBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    swapBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    swapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    swapBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swapBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swapBarrier.image = gVulkan.swapchainImages[imageIndex];
    swapBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    swapBarrier.subresourceRange.levelCount = 1;
    swapBarrier.subresourceRange.layerCount = 1;
    swapBarrier.srcAccessMask = 0;
    swapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(gVulkan.commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapBarrier);

    // Blit with linear filter to upscale
    VkImageBlit blit{};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.layerCount = 1;
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {static_cast<int32_t>(gVulkan.internalExtent.width), static_cast<int32_t>(gVulkan.internalExtent.height), 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.layerCount = 1;
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {static_cast<int32_t>(gVulkan.swapchainExtent.width), static_cast<int32_t>(gVulkan.swapchainExtent.height), 1};

    vkCmdBlitImage(gVulkan.commandBuffer, gVulkan.internalImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   gVulkan.swapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit, VK_FILTER_LINEAR);

    // Transition swapchain image for presentation
    swapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    swapBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    swapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    swapBarrier.dstAccessMask = 0;
    vkCmdPipelineBarrier(gVulkan.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapBarrier);

    vkEndCommandBuffer(gVulkan.commandBuffer);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
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
    return gVulkan.frameSurface;
}

SDL_Surface* vulkan_render_get_texture_surface()
{
    return gVulkan.frameTextureSurface;
}

} // namespace fallout
