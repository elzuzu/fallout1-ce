#include "render/render.h"

#include "plib/gnw/svga.h"

namespace fallout {

static RenderBackend gBackend = RenderBackend::SDL;

bool render_init(RenderBackend backend, VideoOptions* options)
{
    gBackend = backend;

    // Only SDL backend available for now
    return svga_init(options);
}

void render_exit()
{
    svga_exit();
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
    handleWindowSizeChanged();
}

void render_present()
{
    renderPresent();
}

void render_set_window_title(const char* title)
{
    if (gSdlWindow != nullptr) {
        SDL_SetWindowTitle(gSdlWindow, title);
    }
}

SDL_Surface* render_get_surface()
{
    return gSdlSurface;
}

SDL_Surface* render_get_texture_surface()
{
    return gSdlTextureSurface;
}

} // namespace fallout
