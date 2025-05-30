#ifndef FALLOUT_GRAPHICS_RENDER_OBJECT_H_
#define FALLOUT_GRAPHICS_RENDER_OBJECT_H_

#include "graphics/SpriteTypes.h"
#include "graphics/RenderableTypes.h"

namespace fallout {
namespace graphics {

// Type of object to render
enum class ObjectType {
    SPRITE,
    MODEL
};

// Mode forcing sprite or model rendering
enum class RenderMode {
    AUTOMATIC,
    FORCE_SPRITE,
    FORCE_MODEL
};

struct Transform {
    Vec3 position{0, 0, 0};
    Vec4 rotation{0, 0, 0, 1};
    Vec3 scale{1, 1, 1};
};

using MaterialID = int;

struct RenderObject {
    ObjectType type = ObjectType::SPRITE;
    union {
        SpriteData* sprite;
        ModelAsset* model;
    } data{nullptr};
    Transform transform{};
    MaterialID material = 0;
};

// Unified rendering entry point (stub)
void render_object(RenderObject* obj, RenderMode mode);

} // namespace graphics
} // namespace fallout

#endif // FALLOUT_GRAPHICS_RENDER_OBJECT_H_
