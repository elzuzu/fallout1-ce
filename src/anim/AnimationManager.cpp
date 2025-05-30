#include "AnimationManager.h"
#include <iostream> // For placeholder messages

namespace fallout {
namespace anim {

AnimationManager::AnimationManager() {
    // std::cout << "AnimationManager created." << std::endl;
}

AnimationManager::~AnimationManager() {
    ClearAllAnimations();
    // std::cout << "AnimationManager destroyed." << std::endl;
}

// Placeholder implementation for loading/adding an AnimationSet
bool AnimationManager::LoadAnimationSet(const std::string& setName, const AnimationSet& animSet) {
    if (loadedAnimationSets_.count(setName)) {
        std::cerr << "Warning: AnimationSet '" << setName << "' already loaded. Overwriting." << std::endl;
    }

    // In a real system, 'animSet' might be populated by parsing files here.
    // For this placeholder, we assume animSet is already populated.
    // We need to ensure all sequences within the set have their totalDuration calculated.
    AnimationSet setToStore = animSet; // Make a copy to modify
    setToStore.name = setName; // Ensure the set's internal name matches the key
    for (auto& pair : setToStore.sequences) {
        pair.second.CalculateTotalDuration(); // Ensure total duration is up-to-date
    }

    loadedAnimationSets_[setName] = setToStore;
    std::cout << "AnimationSet '" << setName << "' registered in AnimationManager." << std::endl;
    return true;
}

const AnimationSet* AnimationManager::GetAnimationSet(const std::string& setName) const {
    auto it = loadedAnimationSets_.find(setName);
    if (it != loadedAnimationSets_.end()) {
        return &it->second;
    }
    // std::cerr << "Error: AnimationSet '" << setName << "' not found." << std::endl;
    return nullptr;
}

const AnimationSequence* AnimationManager::GetAnimationSequence(const std::string& setName, const std::string& sequenceName) const {
    const AnimationSet* animSet = GetAnimationSet(setName);
    if (animSet) {
        auto seqIt = animSet->sequences.find(sequenceName);
        if (seqIt != animSet->sequences.end()) {
            return &seqIt->second;
        }
        // std::cerr << "Error: AnimationSequence '" << sequenceName << "' not found in set '" << setName << "'." << std::endl;
    }
    return nullptr;
}

void AnimationManager::ClearAllAnimations() {
    loadedAnimationSets_.clear();
    // std::cout << "All animations cleared from AnimationManager." << std::endl;
}

} // namespace anim
} // namespace fallout
