#ifndef FALLOUT_ANIM_ANIMATION_MANAGER_H_
#define FALLOUT_ANIM_ANIMATION_MANAGER_H_

#include "AnimationData.h"
#include <string>
#include <map>
#include <memory> // For std::unique_ptr if managing complex data internally

namespace fallout {
namespace anim {

class AnimationManager {
public:
    AnimationManager();
    ~AnimationManager();

    // Loads an AnimationSet (e.g., from a definition file or multiple FRM files).
    // For now, this will be a placeholder to manually add AnimationSets.
    // In a real system, filePath might point to a meta-file defining an AnimationSet
    // or a directory to scan for FRM files.
    bool LoadAnimationSet(const std::string& setName, const AnimationSet& animSet); // Placeholder for now
    // bool LoadAnimationSetFromPath(const std::string& filePath); // Future real loading

    // Retrieves a complete AnimationSet.
    const AnimationSet* GetAnimationSet(const std::string& setName) const;

    // Retrieves a specific AnimationSequence from a named AnimationSet.
    const AnimationSequence* GetAnimationSequence(const std::string& setName, const std::string& sequenceName) const;

    // Clears all loaded animation data.
    void ClearAllAnimations();

private:
    std::map<std::string, AnimationSet> loadedAnimationSets_;

    // Placeholder for FRM loading logic if it were here:
    // bool ParseFrmFile(const std::string& frmFilePath, AnimationSet& outAnimationSet);
};

} // namespace anim
} // namespace fallout

#endif // FALLOUT_ANIM_ANIMATION_MANAGER_H_
