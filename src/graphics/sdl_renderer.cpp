#include "graphics/renderer_interface.h"
#include "plib/gnw/svga.h"

namespace fallout {

class SDLRenderer : public IRenderer {
public:
    bool initialize(SDL_Window* /*window*/) override {
        // Existing initialization handled elsewhere.
        return true;
    }

    void render() override {
        renderPresent();
    }

    void resize(int /*width*/, int /*height*/) override {
        handleWindowSizeChanged();
    }

    void shutdown() override {
        svga_exit();
    }
};

} // namespace fallout
