#ifndef FALLOUT_RENDER_RENDER_H_
#define FALLOUT_RENDER_RENDER_H_

#include <SDL.h>

#include "plib/gnw/svga_types.h"

namespace fallout {

enum class RenderBackend {
    SDL,
    VULKAN,
};

bool render_init(RenderBackend backend, VideoOptions* options);
void render_exit();

int render_get_width();
int render_get_height();

void render_handle_window_size_changed();
void render_present();

void render_set_window_title(const char* title);

SDL_Surface* render_get_surface();
SDL_Surface* render_get_texture_surface();

} // namespace fallout

#endif /* FALLOUT_RENDER_RENDER_H_ */
