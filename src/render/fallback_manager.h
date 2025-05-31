#pragma once

#include <string>
#include <SDL.h>

#include "render/render.h"

namespace fallout {

// Preferred backend when initializing the renderer.
enum class BackendPreference {
    AUTO,
    VULKAN_ONLY,
    SDL_ONLY,
};

class VulkanManager {
public:
    enum class InitResult { SUCCESS, FAILURE };

    InitResult initialize(VideoOptions* options, bool enableValidation);
    const std::string& getLastError() const { return lastError_; }

private:
    std::string lastError_;
};

class FallbackManager {
public:
    bool initialize(SDL_Window* window,
                    VideoOptions* options,
                    BackendPreference preferred = BackendPreference::AUTO);
    void renderFrame();
    void handleWindowResize(uint32_t width, uint32_t height);
    std::string getRendererInfo() const;
    RenderBackend getActiveBackend() const { return activeBackend_; }
    bool vulkanInitializationFailed() const { return vulkanFailed_; }

private:
    bool initializeVulkanResources(VideoOptions* options);
    bool initializeSDLRenderer(VideoOptions* options);
    void renderFrameVulkan();
    void renderFrameSDL();
    void handleVulkanResize(uint32_t width, uint32_t height);
    void handleSDLResize(uint32_t width, uint32_t height);

    VulkanManager vulkanManager_;
    RenderBackend activeBackend_ = RenderBackend::SDL;
    BackendPreference preferred_ = BackendPreference::AUTO;
    bool vulkanFailed_ = false;
};

} // namespace fallout

