#ifndef FALLOUT_GAME_RESOURCE_MANAGER_H_
#define FALLOUT_GAME_RESOURCE_MANAGER_H_

#include <string>
#include <map>
#include <memory>
#include "graphics/RenderableTypes.h" // For ModelAsset
#include "anim/AnimationData.h"      // For AnimationSet
#include "cache/AssetCache.h"        // For AssetCache
#include "game/AssetConfig.h"        // For AssetConfig

namespace fallout {

// TODO: SpriteData struct should be moved to its own header (e.g. graphics/SpriteTypes.h)
// or to graphics/RenderableTypes.h to avoid circular dependencies or include issues
// if other systems need it, and for AssetCache to correctly use it without fwd declaration tricks.
// For this task, keeping it here to minimize file changes, but AssetCache.cpp will need this definition.
struct SpriteData {
    std::string filePath; // Original path, might be from config
    bool loaded = false;
    // Actual sprite data would go here: e.g. texture ID, dimensions, UVs for a specific sprite,
    // potentially a pointer to a larger texture atlas if sprites are packed.
    // For FRM, this might hold processed frame data or reference to FRM file.
    // For now, it's simple, but AssetCache uses shared_ptr<SpriteData>.
};


class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    // Initialization with config file path
    bool Initialize(const std::string& configFilePath);

    // Sprite Management
    // resourceId is now a logical name (e.g., "Stimpack", "PlayerIdleSprite")
    std::shared_ptr<SpriteData> LoadSprite(const std::string& resourceId, const std::string& category = "GeneralSprites");
    // Unloading might be handled by cache policies or explicitly, but less direct via RM now.
    // void UnloadSprite(const std::string& resourceId);
    std::shared_ptr<graphics::ModelAsset> LoadModel(const std::string& resourceId, const std::string& category = "GeneralModels");
    std::shared_ptr<anim::AnimationSet> LoadAnimationSet(const std::string& resourceId, const std::string& category = "GeneralAnimations");
    std::shared_ptr<graphics::TextureAsset> LoadTexture(const std::string& texturePath, const std::string& category = "Textures"); // Path can be logical ID from now on

    // Accessors now primarily go through the cache, or use config to resolve names.
    // Getters might return shared_ptr directly from cache.
    std::shared_ptr<SpriteData> GetSprite(const std::string& resourceId);
    std::shared_ptr<graphics::ModelAsset> GetModel(const std::string& resourceId);
    std::shared_ptr<anim::AnimationSet> GetAnimationSet(const std::string& resourceId);
    std::shared_ptr<graphics::TextureAsset> GetTexture(const std::string& texturePath);


    // Clear all cached resources
    void ClearCache();
    // Get underlying config and cache if direct access is needed by other systems (use sparingly)
    AssetConfig* GetConfig() const { return assetConfig_.get(); }
    cache::AssetCache* GetCache() const { return assetCache_.get(); }


private:
    std::unique_ptr<cache::AssetCache> assetCache_;
    std::unique_ptr<AssetConfig> assetConfig_;
    bool initialized_ = false;

    // GltfLoader instance might be here or used statically/locally in LoadModel
    // graphics::GltfLoader gltfLoader_; // If it's a member
};

} // namespace fallout

#endif // FALLOUT_GAME_RESOURCE_MANAGER_H_
