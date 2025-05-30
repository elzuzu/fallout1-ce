#include "graphics/renderer_interface.h"
#include <memory>

namespace fallout {

class SDLRenderer; // forward declaration
class VulkanBackendRenderer; // forward declaration

static void saveRendererChoice(const char* /*backend*/) {
    // TODO: persist choice in configuration
}

class VulkanBackendRenderer : public IRenderer {
public:
    bool initialize(SDL_Window* /*window*/) override { return false; }
    void render() override {}
    void resize(int /*width*/, int /*height*/) override {}
    void shutdown() override {}
};

std::unique_ptr<IRenderer> RendererFactory_create(SDL_Window* window) {
    auto vulkanRenderer = std::make_unique<VulkanBackendRenderer>();
    if (vulkanRenderer->initialize(window)) {
        saveRendererChoice("VULKAN");
        return vulkanRenderer;
    }
    auto sdlRenderer = std::make_unique<SDLRenderer>();
    if (sdlRenderer->initialize(window)) {
        saveRendererChoice("SDL");
        return sdlRenderer;
    }
    return nullptr;
}

} // namespace fallout
