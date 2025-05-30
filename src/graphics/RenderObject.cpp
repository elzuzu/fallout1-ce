#include "graphics/RenderObject.h"

namespace fallout {
namespace graphics {

void render_object(RenderObject* obj, RenderMode mode)
{
    if (!obj)
        return;

    ObjectType effectiveType = obj->type;
    if (mode == RenderMode::FORCE_SPRITE)
        effectiveType = ObjectType::SPRITE;
    else if (mode == RenderMode::FORCE_MODEL)
        effectiveType = ObjectType::MODEL;

    // TODO: integrate with SDL or Vulkan renderers
    (void)effectiveType;
}

} // namespace graphics
} // namespace fallout
