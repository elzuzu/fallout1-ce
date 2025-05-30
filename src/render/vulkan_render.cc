#include "render/vulkan_render.h"

namespace fallout {

bool vulkan_render_init(VideoOptions* /*options*/) {
    // TODO: Implement Vulkan renderer initialization
    return false;
}

void vulkan_render_exit() {
    // TODO: Implement Vulkan renderer cleanup
}

void vulkan_render_handle_window_size_changed() {
    // TODO: React to window size changes
}

void vulkan_render_present() {
    // TODO: Present frame using Vulkan
}

SDL_Surface* vulkan_render_get_surface() {
    return nullptr;
}

SDL_Surface* vulkan_render_get_texture_surface() {
    return nullptr;
}

} // namespace fallout
