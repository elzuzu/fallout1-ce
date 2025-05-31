#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <cstring>

namespace fallout {

struct DiabloModel; // Forward declaration for model extraction

struct DiabloBone {
    std::string name;
    int32_t parentIndex = -1;
    glm::mat4 bindPose = glm::mat4(1.0f);
    glm::mat4 inverseBindPose = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::mat4 finalTransform = glm::mat4(1.0f);
};

struct DiabloKeyframe {
    float time;
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
};

struct DiabloAnimationTrack {
    uint32_t boneIndex;
    std::vector<DiabloKeyframe> keyframes;

    glm::vec3 interpolatePosition(float time) const;
    glm::quat interpolateRotation(float time) const;
    glm::vec3 interpolateScale(float time) const;
};

struct DiabloAnimation {
    std::string name;
    float duration = 0.0f;
    float ticksPerSecond = 24.0f;
    std::vector<DiabloAnimationTrack> tracks;
    bool loops = false;
};

struct DiabloSkeleton {
    std::vector<DiabloBone> bones;
    std::unordered_map<std::string, uint32_t> boneNameToIndex;
    glm::mat4 globalInverseTransform = glm::mat4(1.0f);

    std::vector<DiabloAnimation> animations;
    std::unordered_map<std::string, uint32_t> animationNameToIndex;

    static constexpr uint32_t MAX_BONES = 256;
    alignas(16) glm::mat4 boneMatrices[MAX_BONES];

    VkBuffer boneBuffer = VK_NULL_HANDLE;
    VmaAllocation boneAllocation{};
    void* boneMapped = nullptr;
};

struct DiabloAnimationState {
    uint32_t currentAnimation = 0;
    float currentTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool playing = false;
    bool finished = false;

    struct BlendState {
        uint32_t fromAnimation = 0;
        uint32_t toAnimation = 0;
        float blendTime = 0.0f;
        float blendDuration = 0.3f;
        bool blending = false;
    } blend;
};

class DiabloAnimationSystem {
private:
    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator{};

    std::vector<std::unique_ptr<DiabloSkeleton>> skeletons;
    std::vector<DiabloAnimationState> animationStates;

    VkBuffer globalBoneBuffer = VK_NULL_HANDLE;
    VmaAllocation globalBoneAllocation{};
    void* globalBoneMapped = nullptr;

    uint32_t maxInstances = 1000;
    uint32_t activeInstances = 0;

public:
    bool initialize(VkDevice dev, VmaAllocator alloc, uint32_t maxAnimatedCharacters = 1000) {
        device = dev;
        allocator = alloc;
        maxInstances = maxAnimatedCharacters;
        skeletons.reserve(maxInstances);
        animationStates.reserve(maxInstances);
        if (!createGlobalBoneBuffer()) {
            return false;
        }
        return true;
    }

    uint32_t loadSkeleton(const DiabloModel& model) {
        if (activeInstances >= maxInstances) {
            return UINT32_MAX;
        }
        auto skeleton = std::make_unique<DiabloSkeleton>();
        if (!extractSkeletonFromModel(model, *skeleton)) {
            return UINT32_MAX;
        }
        if (!createSkeletonBuffer(*skeleton)) {
            return UINT32_MAX;
        }
        uint32_t id = activeInstances++;
        skeletons.push_back(std::move(skeleton));
        animationStates.emplace_back();
        return id;
    }

    void playAnimation(uint32_t instanceID, const std::string& animationName, bool loop = false, float speed = 1.0f) {
        if (instanceID >= activeInstances) return;
        auto& skeleton = *skeletons[instanceID];
        auto& state = animationStates[instanceID];
        auto it = skeleton.animationNameToIndex.find(animationName);
        if (it == skeleton.animationNameToIndex.end()) return;
        state.currentAnimation = it->second;
        state.currentTime = 0.0f;
        state.playbackSpeed = speed;
        state.playing = true;
        state.finished = false;
        skeleton.animations[state.currentAnimation].loops = loop;
    }

    void blendToAnimation(uint32_t instanceID, const std::string& animationName, float blendDuration = 0.3f) {
        if (instanceID >= activeInstances) return;
        auto& skeleton = *skeletons[instanceID];
        auto& state = animationStates[instanceID];
        auto it = skeleton.animationNameToIndex.find(animationName);
        if (it == skeleton.animationNameToIndex.end()) return;
        state.blend.fromAnimation = state.currentAnimation;
        state.blend.toAnimation = it->second;
        state.blend.blendTime = 0.0f;
        state.blend.blendDuration = blendDuration;
        state.blend.blending = true;
    }

    void updateAnimations(float deltaTime) {
        for (uint32_t i = 0; i < activeInstances; ++i) {
            updateAnimation(i, deltaTime);
        }
        uploadBoneMatrices();
    }

    VkBuffer getBoneBuffer() const { return globalBoneBuffer; }
    uint32_t getBoneMatrixOffset(uint32_t instanceID) const {
        return instanceID * DiabloSkeleton::MAX_BONES * sizeof(glm::mat4);
    }
    bool isAnimationFinished(uint32_t instanceID) const {
        if (instanceID >= activeInstances) return true;
        return animationStates[instanceID].finished;
    }
    float getAnimationProgress(uint32_t instanceID) const {
        if (instanceID >= activeInstances) return 0.0f;
        const auto& state = animationStates[instanceID];
        const auto& anim = skeletons[instanceID]->animations[state.currentAnimation];
        return (anim.duration > 0.0f) ? (state.currentTime / anim.duration) : 0.0f;
    }

private:
    bool createGlobalBoneBuffer() {
        VkDeviceSize bufferSize = maxInstances * DiabloSkeleton::MAX_BONES * sizeof(glm::mat4);
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = bufferSize;
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo result;
        if (vmaCreateBuffer(allocator, &info, &allocInfo, &globalBoneBuffer, &globalBoneAllocation, &result) != VK_SUCCESS) {
            return false;
        }
        globalBoneMapped = result.pMappedData;
        return true;
    }

    bool createSkeletonBuffer(DiabloSkeleton& skeleton) {
        VkDeviceSize bufferSize = DiabloSkeleton::MAX_BONES * sizeof(glm::mat4);
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = bufferSize;
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        VmaAllocationInfo result;
        if (vmaCreateBuffer(allocator, &info, &allocInfo, &skeleton.boneBuffer, &skeleton.boneAllocation, &result) != VK_SUCCESS) {
            return false;
        }
        skeleton.boneMapped = result.pMappedData;
        for (uint32_t i = 0; i < DiabloSkeleton::MAX_BONES; ++i) {
            skeleton.boneMatrices[i] = glm::mat4(1.0f);
        }
        return true;
    }

    bool extractSkeletonFromModel(const DiabloModel& model, DiabloSkeleton& skeleton) {
        if (!model.hasSkeleton()) {
            return false;
        }
        const auto& gltfSkeleton = model.getSkeleton();
        skeleton.bones.resize(gltfSkeleton.bones.size());
        for (size_t i = 0; i < gltfSkeleton.bones.size(); ++i) {
            const auto& gltfBone = gltfSkeleton.bones[i];
            auto& bone = skeleton.bones[i];
            bone.name = gltfBone.name;
            bone.parentIndex = gltfBone.parentIndex;
            bone.bindPose = gltfBone.bindPose;
            bone.inverseBindPose = glm::inverse(gltfBone.bindPose);
            skeleton.boneNameToIndex[bone.name] = static_cast<uint32_t>(i);
        }
        skeleton.globalInverseTransform = gltfSkeleton.globalInverseTransform;

        const auto& gltfAnimations = model.getAnimations();
        skeleton.animations.resize(gltfAnimations.size());
        for (size_t i = 0; i < gltfAnimations.size(); ++i) {
            const auto& gltfAnim = gltfAnimations[i];
            auto& anim = skeleton.animations[i];
            anim.name = gltfAnim.name;
            anim.duration = gltfAnim.duration;
            anim.ticksPerSecond = gltfAnim.ticksPerSecond;

            anim.tracks.resize(gltfAnim.tracks.size());
            for (size_t j = 0; j < gltfAnim.tracks.size(); ++j) {
                const auto& gltfTrack = gltfAnim.tracks[j];
                auto& track = anim.tracks[j];
                track.boneIndex = gltfTrack.boneIndex;
                track.keyframes.resize(gltfTrack.keyframes.size());
                for (size_t k = 0; k < gltfTrack.keyframes.size(); ++k) {
                    const auto& gltfKey = gltfTrack.keyframes[k];
                    auto& key = track.keyframes[k];
                    key.time = gltfKey.time;
                    key.position = gltfKey.position;
                    key.rotation = gltfKey.rotation;
                    key.scale = gltfKey.scale;
                }
            }
            skeleton.animationNameToIndex[anim.name] = static_cast<uint32_t>(i);
        }

        return true;
    }

    void updateAnimation(uint32_t instanceID, float deltaTime) {
        if (instanceID >= activeInstances) return;
        auto& skeleton = *skeletons[instanceID];
        auto& state = animationStates[instanceID];
        if (!state.playing && !state.blend.blending) return;
        if (state.blend.blending) {
            updateBlendedAnimation(skeleton, state, deltaTime);
        } else {
            updateSingleAnimation(skeleton, state, deltaTime);
        }
        calculateBoneMatrices(skeleton);
    }

    void updateSingleAnimation(DiabloSkeleton& skeleton, DiabloAnimationState& state, float deltaTime) {
        if (!state.playing) return;
        const auto& animation = skeleton.animations[state.currentAnimation];
        state.currentTime += deltaTime * state.playbackSpeed;
        if (state.currentTime >= animation.duration) {
            if (animation.loops) {
                state.currentTime = fmod(state.currentTime, animation.duration);
            } else {
                state.currentTime = animation.duration;
                state.playing = false;
                state.finished = true;
            }
        }
        applyAnimationToBones(skeleton, animation, state.currentTime);
    }

    void updateBlendedAnimation(DiabloSkeleton& skeleton, DiabloAnimationState& state, float deltaTime) {
        state.blend.blendTime += deltaTime;
        float blendFactor = glm::clamp(state.blend.blendTime / state.blend.blendDuration, 0.0f, 1.0f);
        std::vector<glm::mat4> fromPose(skeleton.bones.size());
        std::vector<glm::mat4> toPose(skeleton.bones.size());
        const auto& fromAnim = skeleton.animations[state.blend.fromAnimation];
        applyAnimationToBones(skeleton, fromAnim, state.currentTime);
        for (size_t i = 0; i < skeleton.bones.size(); ++i) {
            fromPose[i] = skeleton.bones[i].finalTransform;
        }
        const auto& toAnim = skeleton.animations[state.blend.toAnimation];
        applyAnimationToBones(skeleton, toAnim, 0.0f);
        for (size_t i = 0; i < skeleton.bones.size(); ++i) {
            toPose[i] = skeleton.bones[i].finalTransform;
        }
        for (size_t i = 0; i < skeleton.bones.size(); ++i) {
            glm::vec3 fromPos, toPos, scale1, scale2;
            glm::quat fromRot, toRot;
            glm::vec3 skew; glm::vec4 perspective;
            glm::decompose(fromPose[i], scale1, fromRot, fromPos, skew, perspective);
            glm::decompose(toPose[i], scale2, toRot, toPos, skew, perspective);
            glm::vec3 finalPos = glm::mix(fromPos, toPos, blendFactor);
            glm::quat finalRot = glm::slerp(fromRot, toRot, blendFactor);
            glm::vec3 finalScale = glm::mix(scale1, scale2, blendFactor);
            skeleton.bones[i].finalTransform =
                glm::translate(glm::mat4(1.0f), finalPos) *
                glm::mat4_cast(finalRot) *
                glm::scale(glm::mat4(1.0f), finalScale);
        }
        if (blendFactor >= 1.0f) {
            state.currentAnimation = state.blend.toAnimation;
            state.currentTime = 0.0f;
            state.playing = true;
            state.blend.blending = false;
        }
    }

    void applyAnimationToBones(DiabloSkeleton& skeleton, const DiabloAnimation& animation, float time) {
        for (auto& bone : skeleton.bones) {
            bone.position = glm::vec3(0.0f);
            bone.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            bone.scale = glm::vec3(1.0f);
        }
        for (const auto& track : animation.tracks) {
            if (track.boneIndex >= skeleton.bones.size()) continue;
            auto& bone = skeleton.bones[track.boneIndex];
            bone.position = track.interpolatePosition(time);
            bone.rotation = track.interpolateRotation(time);
            bone.scale = track.interpolateScale(time);
        }
    }

    void calculateBoneMatrices(DiabloSkeleton& skeleton) {
        for (size_t i = 0; i < skeleton.bones.size(); ++i) {
            auto& bone = skeleton.bones[i];
            glm::mat4 localTransform =
                glm::translate(glm::mat4(1.0f), bone.position) *
                glm::mat4_cast(bone.rotation) *
                glm::scale(glm::mat4(1.0f), bone.scale);
            if (bone.parentIndex >= 0 && bone.parentIndex < static_cast<int32_t>(skeleton.bones.size())) {
                bone.finalTransform = skeleton.bones[bone.parentIndex].finalTransform * localTransform;
            } else {
                bone.finalTransform = skeleton.globalInverseTransform * localTransform;
            }
            if (i < DiabloSkeleton::MAX_BONES) {
                skeleton.boneMatrices[i] = bone.finalTransform * bone.inverseBindPose;
            }
        }
    }

    void uploadBoneMatrices() {
        if (!globalBoneMapped) return;
        uint8_t* bufferPtr = static_cast<uint8_t*>(globalBoneMapped);
        for (uint32_t i = 0; i < activeInstances; ++i) {
            const auto& skeleton = *skeletons[i];
            uint32_t offset = i * DiabloSkeleton::MAX_BONES * sizeof(glm::mat4);
            std::memcpy(bufferPtr + offset, skeleton.boneMatrices, DiabloSkeleton::MAX_BONES * sizeof(glm::mat4));
        }
    }
};

inline glm::vec3 DiabloAnimationTrack::interpolatePosition(float time) const {
    if (keyframes.empty()) return glm::vec3(0.0f);
    if (keyframes.size() == 1) return keyframes[0].position;
    size_t i = 0;
    while (i < keyframes.size() - 1 && keyframes[i + 1].time <= time) ++i;
    if (i >= keyframes.size() - 1) return keyframes.back().position;
    const auto& key1 = keyframes[i];
    const auto& key2 = keyframes[i + 1];
    float factor = (time - key1.time) / (key2.time - key1.time);
    return glm::mix(key1.position, key2.position, factor);
}

inline glm::quat DiabloAnimationTrack::interpolateRotation(float time) const {
    if (keyframes.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (keyframes.size() == 1) return keyframes[0].rotation;
    size_t i = 0;
    while (i < keyframes.size() - 1 && keyframes[i + 1].time <= time) ++i;
    if (i >= keyframes.size() - 1) return keyframes.back().rotation;
    const auto& key1 = keyframes[i];
    const auto& key2 = keyframes[i + 1];
    float factor = (time - key1.time) / (key2.time - key1.time);
    return glm::slerp(key1.rotation, key2.rotation, factor);
}

inline glm::vec3 DiabloAnimationTrack::interpolateScale(float time) const {
    if (keyframes.empty()) return glm::vec3(1.0f);
    if (keyframes.size() == 1) return keyframes[0].scale;
    size_t i = 0;
    while (i < keyframes.size() - 1 && keyframes[i + 1].time <= time) ++i;
    if (i >= keyframes.size() - 1) return keyframes.back().scale;
    const auto& key1 = keyframes[i];
    const auto& key2 = keyframes[i + 1];
    float factor = (time - key1.time) / (key2.time - key1.time);
    return glm::mix(key1.scale, key2.scale, factor);
}

} // namespace fallout

