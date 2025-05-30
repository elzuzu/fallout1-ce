#include "AnimationPlayer.h"
#include <cstddef> // For nullptr

namespace fallout {
namespace anim {

AnimationPlayer::AnimationPlayer() = default;

void AnimationPlayer::Play(const AnimationSequence* sequence, bool forceRestart) {
    if (!sequence || sequence->frames.empty()) {
        Stop();
        return;
    }

    if (currentSequence_ == sequence && !forceRestart && isPlaying_) {
        // Already playing this sequence, do nothing unless forced to restart
        return;
    }

    currentSequence_ = sequence;
    currentTime_ = 0.0f;
    currentFrameIndex_ = 0;
    isPlaying_ = true;

    // Ensure total duration is calculated if it wasn't already
    // (Though ideally it's done at load time)
    // if (currentSequence_->totalDuration == 0.0f && !currentSequence_->frames.empty()) {
    //    const_cast<AnimationSequence*>(currentSequence_)->CalculateTotalDuration();
    // }
}

void AnimationPlayer::Stop() {
    isPlaying_ = false;
    currentSequence_ = nullptr;
    currentTime_ = 0.0f;
    currentFrameIndex_ = 0;
}

void AnimationPlayer::Pause() {
    isPlaying_ = false;
}

void AnimationPlayer::Resume() {
    if (currentSequence_ != nullptr) {
        isPlaying_ = true;
    }
}

void AnimationPlayer::Update(float deltaTime) {
    if (!isPlaying_ || !currentSequence_ || currentSequence_->frames.empty() || currentSequence_->totalDuration <= 0.0f) {
        return;
    }

    currentTime_ += deltaTime * playbackSpeed_;

    if (currentSequence_->isLooping) {
        // Handle looping: wrap currentTime_ around totalDuration
        if (currentTime_ >= currentSequence_->totalDuration) {
            // Modulo for precise looping, handles large deltaTime skips too
            currentTime_ = fmod(currentTime_, currentSequence_->totalDuration);
            // If currentTime_ becomes exactly totalDuration due to fmod (e.g. if totalDuration is multiple of itself),
            // it might skip to frame 0 when it should be the last frame for one tick.
            // However, the frame finding logic below should handle it.
            currentFrameIndex_ = 0; // Reset to start for next loop, frame logic will find correct one
        }
    } else { // Not looping
        if (currentTime_ >= currentSequence_->totalDuration) {
            currentTime_ = currentSequence_->totalDuration;
            // Set to last frame
            currentFrameIndex_ = static_cast<int>(currentSequence_->frames.size() - 1);
            isPlaying_ = false; // Animation finished
            // Optionally, call a callback or set a flag here: OnAnimationFinished()
            return; // No further frame updates needed
        }
    }

    // Determine current frame based on currentTime_
    float timeAccumulator = 0.0f;
    int newFrameIndex = 0; // Default to first frame
    bool frameFound = false;

    for (size_t i = 0; i < currentSequence_->frames.size(); ++i) {
        const AnimationFrame& frame = currentSequence_->frames[i];
        if (currentTime_ >= timeAccumulator && currentTime_ < timeAccumulator + frame.duration) {
            newFrameIndex = static_cast<int>(i);
            frameFound = true;
            break;
        }
        timeAccumulator += frame.duration;
    }

    // If, due to looping or timing, currentTime_ lands exactly on or slightly past totalDuration,
    // and we haven't found a frame (e.g. if all frame durations are tiny), default to last frame if looping, or handle as non-looping end.
    if (!frameFound) {
        if (currentSequence_->isLooping) {
             // This can happen if currentTime_ is exactly totalDuration or very slightly more after fmod.
             // Or if totalDuration is 0.
             // Default to last frame of loop, or first if that seems more appropriate.
             // Given the fmod, it's likely the time points to the beginning of the loop.
            newFrameIndex = 0;
            // If currentTime_ was exactly totalDuration and then fmod made it 0, the loop above
            // might not catch the last frame if its duration is also 0.
            // A more robust way might be to check if timeAccumulator is very close to totalDuration for last frame.
            // However, the current loop should correctly assign to the first frame if currentTime_ is 0 after fmod.
        } else {
            // This case should be handled by the non-looping logic above.
            // If somehow reached, means animation ended.
            newFrameIndex = static_cast<int>(currentSequence_->frames.size() - 1);
        }
    }

    currentFrameIndex_ = newFrameIndex;
}

const AnimationFrame* AnimationPlayer::GetCurrentVisualFrame() const {
    if (!currentSequence_ || currentFrameIndex_ < 0 || currentFrameIndex_ >= currentSequence_->frames.size()) {
        return nullptr;
    }
    return &currentSequence_->frames[currentFrameIndex_];
}

std::string AnimationPlayer::GetCurrentSequenceName() const {
    if (currentSequence_) {
        return currentSequence_->name;
    }
    return "";
}

float AnimationPlayer::GetCurrentFrameProgress() const {
    if (!isPlaying_ || !currentSequence_ || currentSequence_->frames.empty() || currentFrameIndex_ < 0 || currentFrameIndex_ >= currentSequence_->frames.size()) {
        return 0.0f;
    }

    const AnimationFrame& currentFrame = currentSequence_->frames[currentFrameIndex_];
    if (currentFrame.duration <= 0.0f) {
        return 1.0f; // Frame has no duration, so it's always "complete"
    }

    // Calculate time elapsed since the start of the current_sequence_ up to the beginning of the currentFrameIndex_
    float timeAtStartOfCurrentFrame = 0.0f;
    for (int i = 0; i < currentFrameIndex_; ++i) {
        timeAtStartOfCurrentFrame += currentSequence_->frames[i].duration;
    }

    float timeIntoCurrentFrame = currentTime_ - timeAtStartOfCurrentFrame;
    return timeIntoCurrentFrame / currentFrame.duration;
}


} // namespace anim
} // namespace fallout
