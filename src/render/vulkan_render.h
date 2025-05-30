#ifndef FALLOUT_RENDER_VULKAN_RENDER_H_
#define FALLOUT_RENDER_VULKAN_RENDER_H_

#include <SDL.h>
#include "plib/gnw/svga_types.h"

namespace fallout {

// Initializes the Vulkan renderer. Returns true on success.
bool vulkan_render_init(VideoOptions* options);

// Cleans up Vulkan renderer resources.
void vulkan_render_exit();

// Handles window size change for Vulkan backend.
void vulkan_render_handle_window_size_changed();

// Presents the current frame using Vulkan.
void vulkan_render_present();

// Currently the Vulkan backend does not expose a surface.
// These functions return nullptr until implemented.
SDL_Surface* vulkan_render_get_surface();
SDL_Surface* vulkan_render_get_texture_surface();

} // namespace fallout

#endif /* FALLOUT_RENDER_VULKAN_RENDER_H_ */
