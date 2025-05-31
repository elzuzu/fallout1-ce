#include "render/fallback_manager.h"

#include "plib/gnw/svga.h"
#include "render/vulkan_render.h"

namespace fallout {

VulkanManager::InitResult VulkanManager::initialize(VideoOptions* options, bool enableValidation)
{
    (void)enableValidation;
    if (!vulkan_is_available()) {
        lastError_ = "Vulkan not available";
        return InitResult::FAILURE;
    }
    if (!vulkan_render_init(options)) {
        lastError_ = "Failed to initialize Vulkan renderer";
        return InitResult::FAILURE;
    }
    return InitResult::SUCCESS;
}

bool FallbackManager::initialize(SDL_Window* window, VideoOptions* options, BackendPreference preferred)
{
    (void)window;
    preferred_ = preferred;
    // Try Vulkan first if allowed
    if (preferred != BackendPreference::SDL_ONLY) {
#ifndef NDEBUG
        bool enableValidation = true;
#else
        bool enableValidation = false;
#endif
        auto result = vulkanManager_.initialize(options, enableValidation);
        if (result == VulkanManager::InitResult::SUCCESS) {
            activeBackend_ = RenderBackend::VULKAN_BATCH;
            return initializeVulkanResources(options);
        }
        vulkanFailed_ = true;
        if (preferred == BackendPreference::VULKAN_ONLY) {
            return false;
        }
    }
    if (initializeSDLRenderer(options)) {
        activeBackend_ = RenderBackend::SDL;
        return true;
    }
    return false;
}

void FallbackManager::renderFrame()
{
    if (activeBackend_ == RenderBackend::VULKAN_BATCH) {
        renderFrameVulkan();
    } else {
        renderFrameSDL();
    }
}

void FallbackManager::handleWindowResize(uint32_t width, uint32_t height)
{
    if (activeBackend_ == RenderBackend::VULKAN_BATCH) {
        handleVulkanResize(width, height);
    } else {
        handleSDLResize(width, height);
    }
}

std::string FallbackManager::getRendererInfo() const
{
    if (activeBackend_ == RenderBackend::VULKAN_BATCH) {
        return "Vulkan Renderer";
    }
    return "SDL Software Renderer";
}

bool FallbackManager::initializeVulkanResources(VideoOptions* /*options*/)
{
    // Placeholder: resources are created during vulkan_render_init
    return true;
}

bool FallbackManager::initializeSDLRenderer(VideoOptions* options)
{
    return svga_init(options);
}

void FallbackManager::renderFrameVulkan() { vulkan_render_present(); }

void FallbackManager::renderFrameSDL() { renderPresent(); }

void FallbackManager::handleVulkanResize(uint32_t, uint32_t) { vulkan_render_handle_window_size_changed(); }

void FallbackManager::handleSDLResize(uint32_t, uint32_t) { handleWindowSizeChanged(); }

} // namespace fallout

