#include "AssetCache.h"
#include "game/ResourceManager.h" // Required for SpriteData definition if it's still there.
                                 // Ideally, SpriteData would move to its own header or RenderableTypes.h

namespace fallout {
namespace cache {

AssetCache::AssetCache() = default;
AssetCache::~AssetCache() = default; // std::map and std::shared_ptr handle cleanup

// --- Model Cache ---
bool AssetCache::TryGetModel(const std::string& resourceId, std::shared_ptr<graphics::ModelAsset>& outModel) const {
    // std::lock_guard<std::mutex> lock(modelCacheMutex_); // If thread-safe
    auto it = modelCache_.find(resourceId);
    if (it != modelCache_.end()) {
        outModel = it->second;
        return true;
    }
    return false;
}

void AssetCache::StoreModel(const std::string& resourceId, std::shared_ptr<graphics::ModelAsset> model) {
    // std::lock_guard<std::mutex> lock(modelCacheMutex_);
    modelCache_[resourceId] = model;
}

void AssetCache::RemoveModel(const std::string& resourceId) {
    // std::lock_guard<std::mutex> lock(modelCacheMutex_);
    modelCache_.erase(resourceId);
}

// --- Sprite Cache ---
// Assuming SpriteData is defined. If not, this part would cause a compile error.
// This highlights the need for SpriteData to be properly defined and included.
bool AssetCache::TryGetSprite(const std::string& resourceId, std::shared_ptr<SpriteData>& outSprite) const {
    // std::lock_guard<std::mutex> lock(spriteCacheMutex_);
    auto it = spriteCache_.find(resourceId);
    if (it != spriteCache_.end()) {
        outSprite = it->second;
        return true;
    }
    return false;
}

void AssetCache::StoreSprite(const std::string& resourceId, std::shared_ptr<SpriteData> sprite) {
    // std::lock_guard<std::mutex> lock(spriteCacheMutex_);
    spriteCache_[resourceId] = sprite;
}

void AssetCache::RemoveSprite(const std::string& resourceId) {
    // std::lock_guard<std::mutex> lock(spriteCacheMutex_);
    spriteCache_.erase(resourceId);
}

// --- AnimationSet Cache ---
bool AssetCache::TryGetAnimationSet(const std::string& setName, std::shared_ptr<anim::AnimationSet>& outAnimSet) const {
    // std::lock_guard<std::mutex> lock(animSetCacheMutex_);
    auto it = animSetCache_.find(setName);
    if (it != animSetCache_.end()) {
        outAnimSet = it->second;
        return true;
    }
    return false;
}

void AssetCache::StoreAnimationSet(const std::string& setName, std::shared_ptr<anim::AnimationSet> animSet) {
    // std::lock_guard<std::mutex> lock(animSetCacheMutex_);
    animSetCache_[setName] = animSet;
}

void AssetCache::RemoveAnimationSet(const std::string& setName) {
    // std::lock_guard<std::mutex> lock(animSetCacheMutex_);
    animSetCache_.erase(setName);
}

// --- Texture Cache ---
bool AssetCache::TryGetTexture(const std::string& texturePath, std::shared_ptr<graphics::TextureAsset>& outTexture) const {
    // std::lock_guard<std::mutex> lock(textureCacheMutex_);
    auto it = textureCache_.find(texturePath);
    if (it != textureCache_.end()) {
        outTexture = it->second;
        return true;
    }
    return false;
}

void AssetCache::StoreTexture(const std::string& texturePath, std::shared_ptr<graphics::TextureAsset> texture) {
    // std::lock_guard<std::mutex> lock(textureCacheMutex_);
    textureCache_[texturePath] = texture;
}

void AssetCache::RemoveTexture(const std::string& texturePath) {
    // std::lock_guard<std::mutex> lock(textureCacheMutex_);
    textureCache_.erase(texturePath);
}


// --- General Cache Operations ---
void AssetCache::ClearAll() {
    ClearModels();
    ClearSprites();
    ClearAnimationSets();
    ClearTextures();
}

void AssetCache::ClearModels() {
    // std::lock_guard<std::mutex> lock(modelCacheMutex_);
    modelCache_.clear();
}

void AssetCache::ClearSprites() {
    // std::lock_guard<std::mutex> lock(spriteCacheMutex_);
    spriteCache_.clear();
}

void AssetCache::ClearAnimationSets() {
    // std::lock_guard<std::mutex> lock(animSetCacheMutex_);
    animSetCache_.clear();
}

void AssetCache::ClearTextures() {
    // std::lock_guard<std::mutex> lock(textureCacheMutex_);
    textureCache_.clear();
}

} // namespace cache
} // namespace fallout
