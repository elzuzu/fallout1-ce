#ifndef FALLOUT_ANIM_ANIMATION_PLAYER_H_
#define FALLOUT_ANIM_ANIMATION_PLAYER_H_

#include "AnimationData.h"

namespace fallout {
namespace anim {

class AnimationPlayer {
public:
    AnimationPlayer();

    // Starts playing the given animation sequence.
    // If a sequence is already playing, it will be replaced.
    void Play(const AnimationSequence* sequence, bool forceRestart = false);

    // Stops playback and resets the current animation state.
    void Stop();

    // Pauses playback at the current frame.
    void Pause();

    // Resumes playback from the current frame.
    void Resume();

    // Updates the animation timing. Should be called every game frame.
    // deltaTime is the time elapsed since the last update, in seconds.
    void Update(float deltaTime);

    // Returns true if an animation is currently playing.
    bool IsPlaying() const { return isPlaying_; }

    // Returns the name of the currently playing sequence, or empty string if none.
    std::string GetCurrentSequenceName() const;

    // Sets the playback speed. 1.0 is normal speed.
    void SetPlaybackSpeed(float speed) { playbackSpeed_ = speed; }
    float GetPlaybackSpeed() const { return playbackSpeed_; }

    // Gets the current visual frame to be rendered.
    // This selects the correct frame based on current time, but does not perform visual interpolation between frames.
    const AnimationFrame* GetCurrentVisualFrame() const;

    // For potential future use: if visual interpolation (e.g. cross-fade) is needed.
    // void GetInterpolatedVisualFrame(AnimationFrame& outFrame, AnimationFrame& nextFrame, float& blendFactor) const;

    // Returns the raw current frame index
    int GetCurrentFrameIndex() const { return currentFrameIndex_; }

    // Returns the current progress (0.0 to 1.0) within the current frame's duration
    float GetCurrentFrameProgress() const;


private:
    const AnimationSequence* currentSequence_ = nullptr;
    float currentTime_ = 0.0f;          // Current time within the sequence
    int currentFrameIndex_ = 0;
    bool isPlaying_ = false;
    float playbackSpeed_ = 1.0f;
};

} // namespace anim
} // namespace fallout

#endif // FALLOUT_ANIM_ANIMATION_PLAYER_H_
