#ifndef FALLOUT_GRAPHICS_RENDERER_FACTORY_H_
#define FALLOUT_GRAPHICS_RENDERER_FACTORY_H_

#include <memory>
#include <SDL.h>
#include "graphics/renderer_interface.h"

namespace fallout {

std::unique_ptr<IRenderer> RendererFactory_create(SDL_Window* window);

} // namespace fallout

#endif /* FALLOUT_GRAPHICS_RENDERER_FACTORY_H_ */
