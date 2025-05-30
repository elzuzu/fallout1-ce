#ifndef FALLOUT_CACHE_ASSET_CACHE_H_
#define FALLOUT_CACHE_ASSET_CACHE_H_

#include <string>
#include <map>
#include <memory> // For std::shared_ptr if assets are complex and managed
#include "graphics/RenderableTypes.h" // For ModelAsset
#include "anim/AnimationData.h"      // For AnimationSet (if we cache these)
// Include other processed asset types as needed, e.g., SpriteData if it becomes more complex

namespace fallout {

// Forward declare SpriteData if it's still simple and defined in ResourceManager.h
// If SpriteData becomes complex (e.g., holding texture handles), move it to its own header.
struct SpriteData; // Assuming it's defined elsewhere, e.g., ResourceManager.h for now

namespace cache {

class AssetCache {
public:
    AssetCache();
    ~AssetCache();

    // --- Model Cache ---
    // Using std::shared_ptr for ModelAsset as they can be large and shared.
    bool TryGetModel(const std::string& resourceId, std::shared_ptr<graphics::ModelAsset>& outModel) const;
    void StoreModel(const std::string& resourceId, std::shared_ptr<graphics::ModelAsset> model);
    void RemoveModel(const std::string& resourceId);

    // --- Sprite Cache ---
    // Assuming SpriteData is relatively lightweight or will be. If it holds GPU resources, use shared_ptr.
    // For now, let's assume it might become complex and use shared_ptr too.
    bool TryGetSprite(const std::string& resourceId, std::shared_ptr<SpriteData>& outSprite) const;
    void StoreSprite(const std::string& resourceId, std::shared_ptr<SpriteData> sprite);
    void RemoveSprite(const std::string& resourceId);

    // --- AnimationSet Cache ---
    bool TryGetAnimationSet(const std::string& setName, std::shared_ptr<anim::AnimationSet>& outAnimSet) const;
    void StoreAnimationSet(const std::string& setName, std::shared_ptr<anim::AnimationSet> animSet);
    void RemoveAnimationSet(const std::string& setName);

    // --- Texture Cache ---
    bool TryGetTexture(const std::string& texturePath, std::shared_ptr<graphics::TextureAsset>& outTexture) const;
    void StoreTexture(const std::string& texturePath, std::shared_ptr<graphics::TextureAsset> texture);
    void RemoveTexture(const std::string& texturePath);

    // --- General Cache Operations ---
    void ClearAll();
    void ClearModels();
    void ClearSprites();
    void ClearAnimationSets();
    void ClearTextures();

    // TODO: Add cache policies (e.g., max size, LRU eviction) if needed in the future.

private:
    // Mutexes for thread safety if accessed from multiple threads (not implemented in this scope)
    // mutable std::mutex modelCacheMutex_;
    // mutable std::mutex spriteCacheMutex_;
    // mutable std::mutex animSetCacheMutex_;
    // mutable std::mutex textureCacheMutex_;

    std::map<std::string, std::shared_ptr<graphics::ModelAsset>> modelCache_;
    std::map<std::string, std::shared_ptr<SpriteData>> spriteCache_; // SpriteData might evolve to use TextureAsset or similar
    std::map<std::string, std::shared_ptr<anim::AnimationSet>> animSetCache_;
    std::map<std::string, std::shared_ptr<graphics::TextureAsset>> textureCache_;
};

} // namespace cache
} // namespace fallout

#endif // FALLOUT_CACHE_ASSET_CACHE_H_
