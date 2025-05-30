#include "render/vulkan_render.h"
#include "game/graphics_advanced.h"
#include "graphics/vulkan/MemoryAllocator.hpp"
#include "plib/gnw/svga.h"
#include "render/post_processor.h"
#include "render/vulkan_capabilities.h"
#include "render/vulkan_debugger.h"
#include "render/vulkan_thread_manager.h"
#include <cstdlib>
#include <cstring>

#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <memory>
#include <vector>

namespace fallout {

VulkanRenderer gVulkan;

namespace {

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

        if (gVulkanCaps.maxTextureSize > 0) {
            gVulkan.internalExtent.width = std::min(gVulkan.internalExtent.width, gVulkanCaps.maxTextureSize);
            gVulkan.internalExtent.height = std::min(gVulkan.internalExtent.height, gVulkanCaps.maxTextureSize);
        }

        VkImageCreateInfo imageInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
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

        VkMemoryAllocateInfo allocInfo { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = find_memory_type(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(gVulkan.device, &allocInfo, nullptr, &gVulkan.internalImageMemory) != VK_SUCCESS)
            return false;

        vkBindImageMemory(gVulkan.device, gVulkan.internalImage, gVulkan.internalImageMemory, 0);

        VkImageViewCreateInfo viewInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
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
        swapInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
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

        gVulkan.postProcessor.destroy(gVulkan.device);
        gVulkan.postProcessor.init(gVulkan.device, gVulkan.swapchainImageFormat, gVulkan.swapchainExtent);

        return true;
    }

    static void destroy_swapchain()
    {
        gVulkan.postProcessor.destroy(gVulkan.device);
        for (VkImageView view : gVulkan.swapchainImageViews)
            vkDestroyImageView(gVulkan.device, view, nullptr);
        gVulkan.swapchainImageViews.clear();

        destroy_internal_image();

        if (gVulkan.swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(gVulkan.device, gVulkan.swapchain, nullptr);
            gVulkan.swapchain = VK_NULL_HANDLE;
        }
    }

    static void record_and_submit(uint32_t imageIndex, uint32_t frame, VkCommandBuffer cmdBuffer, VkFence inFlight)
    {
        vkResetCommandBuffer(cmdBuffer, 0);
        VkCommandBufferBeginInfo beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        vkBeginCommandBuffer(cmdBuffer, &beginInfo);

        if (gGraphicsAdvanced.debugger)
            gVulkanDebugger.begin_frame(frame, cmdBuffer);

        VkClearValue clear {};
        clear.color.float32[0] = 0.f;
        clear.color.float32[1] = 0.f;
        clear.color.float32[2] = 0.f;
        clear.color.float32[3] = 1.f;

        VkRenderingAttachmentInfo offscreenAttachment { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        offscreenAttachment.imageView = gVulkan.internalImageView;
        offscreenAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        offscreenAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        offscreenAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        offscreenAttachment.clearValue = clear;

        VkRenderingInfo offscreenInfo { VK_STRUCTURE_TYPE_RENDERING_INFO };
        offscreenInfo.renderArea.offset = { 0, 0 };
        offscreenInfo.renderArea.extent = gVulkan.internalExtent;
        offscreenInfo.layerCount = 1;
        offscreenInfo.colorAttachmentCount = 1;
        offscreenInfo.pColorAttachments = &offscreenAttachment;

        vkCmdBeginRendering(cmdBuffer, &offscreenInfo);
        vkCmdEndRendering(cmdBuffer);

        VkImageMemoryBarrier offscreenBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
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
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &offscreenBarrier);

        VkImageMemoryBarrier swapBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
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
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapBarrier);

        gVulkan.postProcessor.apply(cmdBuffer, gVulkan.internalImage, gVulkan.swapchainImages[imageIndex]);

        swapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        swapBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        swapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        swapBarrier.dstAccessMask = 0;
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &swapBarrier);

        if (gGraphicsAdvanced.debugger)
            gVulkanDebugger.end_frame(frame, cmdBuffer);

        vkEndCommandBuffer(cmdBuffer);

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkSubmitInfo submit { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &gVulkan.imageAvailable[frame];
        submit.pWaitDstStageMask = &waitStage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmdBuffer;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &gVulkan.renderFinished[frame];

        vkQueueSubmit(gVulkan.graphicsQueue, 1, &submit, inFlight);

        VkPresentInfoKHR present { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &gVulkan.renderFinished[frame];
        present.swapchainCount = 1;
        present.pSwapchains = &gVulkan.swapchain;
        present.pImageIndices = &imageIndex;

        vkQueuePresentKHR(gVulkan.graphicsQueue, &present);

        if (gGraphicsAdvanced.debugger)
            gVulkanDebugger.resolve_frame(frame);
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
    if (gGraphicsAdvanced.validation) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        extCount = static_cast<unsigned int>(extensions.size());
    }

    VkInstanceCreateInfo instInfo { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = extCount;
    instInfo.ppEnabledExtensionNames = extensions.data();
    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
    if (gGraphicsAdvanced.validation) {
        instInfo.enabledLayerCount = 1;
        instInfo.ppEnabledLayerNames = layers;
    }

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
    uint32_t index = 0;
    if (gGraphicsAdvanced.gpuIndex < static_cast<int>(gpuCount))
        index = gGraphicsAdvanced.gpuIndex;
    gVulkan.physicalDevice = gpus[index];

    gVulkanCaps.init(gVulkan.physicalDevice);

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
    MemoryAllocator::init(gVulkan.instance, gVulkan.physicalDevice, gVulkan.device, gVulkan.graphicsQueueFamily);

    if (gGraphicsAdvanced.debugger)
        gVulkanDebugger.init(gVulkan.instance, gVulkan.physicalDevice, gVulkan.device);

    // Configure post-processing effects via environment variables
    const char* fxaaEnv = getenv("F1CE_FXAA");
    if (fxaaEnv && strcmp(fxaaEnv, "1") == 0)
        gVulkan.postProcessor.addEffect(std::make_unique<FxaaEffect>());
    const char* smaaEnv = getenv("F1CE_SMAA");
    if (smaaEnv && strcmp(smaaEnv, "1") == 0)
        gVulkan.postProcessor.addEffect(std::make_unique<SmaaEffect>());
    const char* crtEnv = getenv("F1CE_CRT");
    if (crtEnv && strcmp(crtEnv, "1") == 0)
        gVulkan.postProcessor.addEffect(std::make_unique<CrtEffect>());
    const char* slEnv = getenv("F1CE_SCANLINES");
    if (slEnv && strcmp(slEnv, "1") == 0)
        gVulkan.postProcessor.addEffect(std::make_unique<ScanlineEffect>());
    const char* nearestEnv = getenv("F1CE_NEAREST_UPSCALE");
    gVulkan.postProcessor.addEffect(std::make_unique<UpscaleEffect>(nearestEnv && strcmp(nearestEnv, "1") == 0));

    if (!create_swapchain(gVulkan.width, gVulkan.height))
        return false;

    VkCommandPoolCreateInfo poolInfo { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.queueFamilyIndex = gVulkan.graphicsQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    constexpr uint32_t kMaxFramesInFlight = 3;
    gVulkan.commandPools.resize(kMaxFramesInFlight);
    gVulkan.commandBuffers.resize(kMaxFramesInFlight);
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (vkCreateCommandPool(gVulkan.device, &poolInfo, nullptr, &gVulkan.commandPools[i]) != VK_SUCCESS)
            return false;

        VkCommandBufferAllocateInfo cmdInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        cmdInfo.commandPool = gVulkan.commandPools[i];
        cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdInfo.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(gVulkan.device, &cmdInfo, &gVulkan.commandBuffers[i]) != VK_SUCCESS)
            return false;
    }

    VkDescriptorPoolSize poolSize { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxFramesInFlight };
    VkDescriptorPoolCreateInfo descPoolInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    descPoolInfo.poolSizeCount = 1;
    descPoolInfo.pPoolSizes = &poolSize;
    descPoolInfo.maxSets = kMaxFramesInFlight;
    if (vkCreateDescriptorPool(gVulkan.device, &descPoolInfo, nullptr, &gVulkan.descriptorPool) != VK_SUCCESS)
        return false;

    VkPipelineCacheCreateInfo cacheInfo { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
    vkCreatePipelineCache(gVulkan.device, &cacheInfo, nullptr, &gVulkan.pipelineCache);

    VkSemaphoreCreateInfo semInfo { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    gVulkan.imageAvailable.resize(kMaxFramesInFlight);
    gVulkan.renderFinished.resize(kMaxFramesInFlight);
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        vkCreateSemaphore(gVulkan.device, &semInfo, nullptr, &gVulkan.imageAvailable[i]);
        vkCreateSemaphore(gVulkan.device, &semInfo, nullptr, &gVulkan.renderFinished[i]);
    }

    VkFenceCreateInfo fenceInfo { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    gVulkan.inFlightFences.resize(kMaxFramesInFlight);
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        vkCreateFence(gVulkan.device, &fenceInfo, nullptr, &gVulkan.inFlightFences[i]);
    }
    gVulkan.currentFrame = 0;

    if (gGraphicsAdvanced.multithreaded)
        gVulkanThread.start();

    return true;
}

void vulkan_render_exit()
{
    if (gVulkan.device != VK_NULL_HANDLE) {
        if (gGraphicsAdvanced.multithreaded)
            gVulkanThread.stop();
        if (gGraphicsAdvanced.debugger)
            gVulkanDebugger.destroy();
        vkDeviceWaitIdle(gVulkan.device);
        MemoryAllocator::shutdown();

        for (VkFence f : gVulkan.inFlightFences) {
            vkDestroyFence(gVulkan.device, f, nullptr);
        }
        gVulkan.inFlightFences.clear();

        for (VkSemaphore s : gVulkan.renderFinished)
            vkDestroySemaphore(gVulkan.device, s, nullptr);
        for (VkSemaphore s : gVulkan.imageAvailable)
            vkDestroySemaphore(gVulkan.device, s, nullptr);
        gVulkan.renderFinished.clear();
        gVulkan.imageAvailable.clear();

        if (gVulkan.descriptorPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(gVulkan.device, gVulkan.descriptorPool, nullptr);
        if (gVulkan.pipelineCache != VK_NULL_HANDLE)
            vkDestroyPipelineCache(gVulkan.device, gVulkan.pipelineCache, nullptr);

        for (VkCommandPool p : gVulkan.commandPools)
            vkDestroyCommandPool(gVulkan.device, p, nullptr);
        gVulkan.commandPools.clear();

        destroy_swapchain();
    }

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

    gVulkan = {};
}

void vulkan_render_handle_window_size_changed()
{
    if (gVulkan.device == VK_NULL_HANDLE)
        return;

    int w, h;
    SDL_Vulkan_GetDrawableSize(gSdlWindow, &w, &h);
    gVulkan.width = static_cast<uint32_t>(w);
    gVulkan.height = static_cast<uint32_t>(h);

    if (gGraphicsAdvanced.multithreaded)
        gVulkanThread.stop();
    vkDeviceWaitIdle(gVulkan.device);

    destroy_swapchain();
    create_swapchain(gVulkan.width, gVulkan.height);

    if (gGraphicsAdvanced.multithreaded)
        gVulkanThread.start();
}

void vulkan_render_present()
{
    if (gVulkan.device == VK_NULL_HANDLE)
        return;

    VkCommandBuffer cmdBuffer = gVulkan.commandBuffers[gVulkan.currentFrame];
    VkFence inFlight = gVulkan.inFlightFences[gVulkan.currentFrame];

    vkResetCommandPool(gVulkan.device, gVulkan.commandPools[gVulkan.currentFrame], 0);

    vkWaitForFences(gVulkan.device, 1, &inFlight, VK_TRUE, UINT64_MAX);
    vkResetFences(gVulkan.device, 1, &inFlight);

    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(gVulkan.device, gVulkan.swapchain, UINT64_MAX, gVulkan.imageAvailable[gVulkan.currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (!gGraphicsAdvanced.multithreaded) {
        record_and_submit(imageIndex, gVulkan.currentFrame, cmdBuffer, inFlight);
    } else {
        RenderCommand cmd {};
        cmd.func = [imageIndex, frame = gVulkan.currentFrame, cmdBuffer, inFlight]() {
            record_and_submit(imageIndex, frame, cmdBuffer, inFlight);
        };
        gVulkanThread.submit(cmd);
    }

    gVulkan.currentFrame = (gVulkan.currentFrame + 1) % gVulkan.commandBuffers.size();
}

SDL_Surface* vulkan_render_get_surface()
{
    return gVulkan.frameSurface;
}

SDL_Surface* vulkan_render_get_texture_surface()
{
    return gVulkan.frameTextureSurface;
}

bool vulkan_is_available()
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
        return false;

    bool available = SDL_Vulkan_LoadLibrary(nullptr) == 0;
    if (available)
        SDL_Vulkan_UnloadLibrary();

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    return available;
}

} // namespace fallout
