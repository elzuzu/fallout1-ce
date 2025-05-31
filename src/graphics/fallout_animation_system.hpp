#pragma once

#include "graphics/diablo_animation_system.hpp"
#include "render/fallout_memory_manager.h"
#include <unordered_map>
#include <memory>
#include <string>

namespace fallout {


class FalloutAssetManager {
public:
    enum class AssetType {
        MODERN_MESH_3D,
        LEGACY_SPRITE_2D,
    };

    struct RenderableAsset {
        AssetType type;
        // Placeholder for actual renderable asset data
        struct SpriteData { std::vector<void*> frameTextures; } sprite;
        struct MeshData { std::vector<int> primitives; } mesh;
    };

    bool loadAsset(const std::string&, AssetType) { return false; }
    bool isAssetLoaded(const std::string&) const { return false; }
    AssetType getAssetType(const std::string&) const { return AssetType::LEGACY_SPRITE_2D; }
    std::shared_ptr<RenderableAsset> getRenderableAsset(const std::string&) { return nullptr; }
};

class FalloutAnimationSystem : public DiabloAnimationSystem {
    FalloutMemoryManager* memoryManager = nullptr;
    FalloutAssetManager* assetManager = nullptr;
    std::unordered_map<uint32_t, std::string> legacyFallbacks;

public:
    bool initializeForFallout(FalloutMemoryManager* memMgr,
                              FalloutAssetManager* assetMgr,
                              VkDevice dev, VmaAllocator alloc,
                              uint32_t maxAnimatedCharacters = 1000)
    {
        memoryManager = memMgr;
        assetManager = assetMgr;
        return initialize(dev, alloc, maxAnimatedCharacters);
    }

    uint32_t loadSkeletonFromAssets(const std::string& assetName)
    {
        if (!assetManager)
            return UINT32_MAX;
        auto renderable = assetManager->getRenderableAsset(assetName);
        if (renderable && renderable->type == FalloutAssetManager::AssetType::MODERN_MESH_3D)
            return loadSkeletonFromModernAsset(*renderable);
        if (assetManager->isAssetLoaded(assetName) &&
            assetManager->getAssetType(assetName) == FalloutAssetManager::AssetType::LEGACY_SPRITE_2D)
        {
            uint32_t id = createLegacyAnimationInstance(assetName);
            if (id != UINT32_MAX)
                legacyFallbacks[id] = assetName;
            return id;
        }
        return UINT32_MAX;
    }

    void blendToModernAnimation(uint32_t instanceID, const std::string& animationName,
                                float blendDuration = 0.3f)
    {
        auto it = legacyFallbacks.find(instanceID);
        if (it != legacyFallbacks.end()) {
            std::string modernAsset = it->second + "_3d";
            if (assetManager && assetManager->loadAsset(modernAsset,
                                                        FalloutAssetManager::AssetType::MODERN_MESH_3D)) {
                uint32_t newID = loadSkeletonFromAssets(modernAsset);
                if (newID != UINT32_MAX) {
                    transferAnimationState(instanceID, newID);
                    legacyFallbacks.erase(it);
                    legacyFallbacks[newID] = modernAsset;
                    instanceID = newID;
                }
            }
        }
        blendToAnimation(instanceID, animationName, blendDuration);
    }

private:
    uint32_t loadSkeletonFromModernAsset(const FalloutAssetManager::RenderableAsset& asset)
    {
        DiabloModel model; // Placeholder model
        if (!asset.mesh.primitives.empty()) {
            convertglTFToModel(asset, model);
            return loadSkeleton(model);
        }
        return UINT32_MAX;
    }

    uint32_t createLegacyAnimationInstance(const std::string& assetName)
    {
        auto skeleton = std::make_unique<DiabloSkeleton>();
        DiabloBone root{};
        root.name = "sprite_root";
        root.parentIndex = -1;
        root.bindPose = glm::mat4(1.0f);
        root.inverseBindPose = glm::mat4(1.0f);
        skeleton->bones.push_back(root);
        skeleton->boneNameToIndex[root.name] = 0;

        DiabloAnimation anim{};
        anim.name = "sprite_animation";
        anim.duration = 1.0f;
        anim.ticksPerSecond = 10.0f;
        anim.loops = true;

        DiabloAnimationTrack track{};
        track.boneIndex = 0;
        uint32_t frames = getSpriteFrameCount(assetName);
        for (uint32_t i = 0; i < frames; ++i) {
            DiabloKeyframe k{};
            k.time = static_cast<float>(i) / anim.ticksPerSecond;
            k.position = glm::vec3(0.0f);
            k.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            k.scale = glm::vec3(1.0f);
            track.keyframes.push_back(k);
        }
        anim.tracks.push_back(track);
        skeleton->animations.push_back(anim);
        skeleton->animationNameToIndex[anim.name] = 0;

        if (!createSkeletonBuffer(*skeleton))
            return UINT32_MAX;
        uint32_t instanceID = activeInstances++;
        skeletons.push_back(std::move(skeleton));
        animationStates.emplace_back();
        return instanceID;
    }

    void convertglTFToModel(const FalloutAssetManager::RenderableAsset&, DiabloModel&) {}

    uint32_t getSpriteFrameCount(const std::string& assetName)
    {
        if (!assetManager) return 1;
        auto renderable = assetManager->getRenderableAsset(assetName);
        if (renderable && renderable->type == FalloutAssetManager::AssetType::LEGACY_SPRITE_2D)
            return static_cast<uint32_t>(renderable->sprite.frameTextures.size());
        return 1;
    }

    void transferAnimationState(uint32_t fromID, uint32_t toID)
    {
        if (fromID >= activeInstances || toID >= activeInstances)
            return;
        animationStates[toID] = animationStates[fromID];
    }
};

} // namespace fallout

