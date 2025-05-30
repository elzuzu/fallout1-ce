#ifndef FALLOUT_GAME_GRAPHICS_ADVANCED_H_
#define FALLOUT_GAME_GRAPHICS_ADVANCED_H_

namespace fallout {

struct GraphicsAdvancedOptions {
    int gpuIndex;
    int textureQuality;
    int filtering;
    bool vsync;
    int fpsLimit;
    bool validation;
    bool multithreaded;
    bool debugger;
};

extern GraphicsAdvancedOptions gGraphicsAdvanced;

void graphics_advanced_load();
void graphics_advanced_save();
void graphics_advanced_apply();
int do_graphics_advanced();

} // namespace fallout

#endif /* FALLOUT_GAME_GRAPHICS_ADVANCED_H_ */
