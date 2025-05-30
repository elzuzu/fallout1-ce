#ifndef FALLOUT_GRAPHICS_RENDERER_INTERFACE_H_
#define FALLOUT_GRAPHICS_RENDERER_INTERFACE_H_

#include <SDL.h>

namespace fallout {

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool initialize(SDL_Window* window) = 0;
    virtual void render() = 0;
    virtual void resize(int width, int height) = 0;
    virtual void shutdown() = 0;
};

} // namespace fallout

#endif /* FALLOUT_GRAPHICS_RENDERER_INTERFACE_H_ */
