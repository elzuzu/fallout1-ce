#ifndef FALLOUT_ANIM_ANIMATION_DATA_H_
#define FALLOUT_ANIM_ANIMATION_DATA_H_

#include <string>
#include <vector>
#include <map>
// #include "graphics/RenderableTypes.h" // For Vec2 if not defined globally, but it is.

// Using Vec2 from RenderableTypes.h (implicitly via project structure)
// If not, define it:
// struct Vec2 { float x, y; };


namespace fallout {
namespace anim {

// Represents a single frame in an animation sequence
struct AnimationFrame {
    int resourceId;      // ID for the sprite/texture of this frame (e.g., an FRM Frame ID or texture atlas sub-region ID)
    float duration;        // How long this frame should be displayed (in seconds)

    // For sprite sheets / texture atlases:
    Vec2 uvOffset = {0.0f, 0.0f}; // Top-left UV coordinate on the sprite sheet
    Vec2 uvSize = {1.0f, 1.0f};   // Size of the UV area (normalized: 1,1 means full texture)

    Vec2 pivot = {0.5f, 0.5f};    // Normalized pivot point (0,0 top-left, 0.5,0.5 center)
    Vec2 sizePx = {0.0f, 0.0f};   // Original pixel size of the frame (optional, for reference)

    // Future: event_triggers, sound_triggers associated with this frame
};

// Represents a sequence of frames that form a complete animation
struct AnimationSequence {
    std::string name;            // e.g., "critter_walk_east", "item_glow"
    std::vector<AnimationFrame> frames;
    bool isLooping = true;
    float totalDuration = 0.0f; // Calculated automatically

    void CalculateTotalDuration() {
        totalDuration = 0.0f;
        for (const auto& frame : frames) {
            totalDuration += frame.duration;
        }
    }
};

// (Optional) Represents a sprite sheet containing multiple frames for various animations
// This would be analogous to how FRM files often pack frames.
struct SpriteSheet {
    std::string name;
    int baseTextureResourceId; // ID of the large texture atlas

    // Defines regions within the base texture that correspond to logical frames
    // Key could be a local frame index or a specific name/id for that frame part.
    std::map<int, AnimationFrame> frameDefinitionsOnSheet;
};


// Data structure to hold all animations for a particular entity type or group
// e.g., all animations for a "Human Male" critter.
struct AnimationSet {
    std::string name; // e.g., "human_male_animations"
    std::map<std::string, AnimationSequence> sequences; // Keyed by sequence name ("idle_N", "walk_NW", etc.)

    // Future: Could also store paths to FRM files or sprite sheets here
    // std::vector<std::string> frmFilePaths;
};

} // namespace anim
} // namespace fallout

#endif // FALLOUT_ANIM_ANIMATION_DATA_H_
