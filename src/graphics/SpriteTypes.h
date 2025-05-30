#ifndef FALLOUT_GRAPHICS_SPRITE_TYPES_H_
#define FALLOUT_GRAPHICS_SPRITE_TYPES_H_

#include <string>

namespace fallout {

struct SpriteData {
    std::string filePath; // Original path, might be from config
    bool loaded = false;
    // Actual sprite data would go here: e.g. texture ID, dimensions, UVs for a specific sprite,
    // potentially a pointer to a larger texture atlas if sprites are packed.
    // For FRM, this might hold processed frame data or reference to FRM file.
    // For now, it's simple, but AssetCache uses shared_ptr<SpriteData>.
};

} // namespace fallout

#endif // FALLOUT_GRAPHICS_SPRITE_TYPES_H_
