#include "render/render.h"

#include "plib/gnw/svga.h"
#include "render/vulkan_render.h"

namespace fallout {

static RenderBackend gBackend = RenderBackend::SDL;

bool render_init(RenderBackend backend, VideoOptions* options)
{
    gBackend = backend;

    switch (backend) {
    case RenderBackend::SDL:
        return svga_init(options);
    case RenderBackend::VULKAN_BATCH:
        return vulkan_render_init(options);
    }

    return false;
}

void render_exit()
{
    switch (gBackend) {
    case RenderBackend::SDL:
        svga_exit();
        break;
    case RenderBackend::VULKAN_BATCH:
        vulkan_render_exit();
        break;
    }
}

int render_get_width()
{
    return screenGetWidth();
}

int render_get_height()
{
    return screenGetHeight();
}

void render_handle_window_size_changed()
{
    switch (gBackend) {
    case RenderBackend::SDL:
        handleWindowSizeChanged();
        break;
    case RenderBackend::VULKAN_BATCH:
        vulkan_render_handle_window_size_changed();
        break;
    }
}

void render_present()
{
    switch (gBackend) {
    case RenderBackend::SDL:
        renderPresent();
        break;
    case RenderBackend::VULKAN_BATCH:
        vulkan_render_present();
        break;
    }
}

void render_set_window_title(const char* title)
{
    if (gSdlWindow != nullptr) {
        SDL_SetWindowTitle(gSdlWindow, title);
    }
}

SDL_Surface* render_get_surface()
{
    switch (gBackend) {
    case RenderBackend::SDL:
        return gSdlSurface;
    case RenderBackend::VULKAN_BATCH:
        return vulkan_render_get_surface();
    }

    return nullptr;
}

SDL_Surface* render_get_texture_surface()
{
    switch (gBackend) {
    case RenderBackend::SDL:
        return gSdlTextureSurface;
    case RenderBackend::VULKAN_BATCH:
        return vulkan_render_get_texture_surface();
    }

    return nullptr;
}

} // namespace fallout
