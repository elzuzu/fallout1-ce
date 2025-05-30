#include "ResourceManager.h"
#include "graphics/GltfLoader.h" // For loading models
// anim/AnimationManager.h is not directly used by RM, but anim/AnimationData.h is via AssetCache
#include "anim/AnimationData.h"
#include <iostream> // For placeholder messages

// Note: The SpriteData struct definition is currently in ResourceManager.h.
// This is needed for AssetCache's std::shared_ptr<SpriteData>.
// Ideally, move SpriteData to its own header or a common graphics types header.

namespace fallout {

ResourceManager::ResourceManager() : initialized_(false) {
    assetCache_ = std::make_unique<cache::AssetCache>();
    assetConfig_ = std::make_unique<AssetConfig>();
    // GltfLoader could be instantiated here if it's a member and frequently used:
    // gltfLoader_ = std::make_unique<graphics::GltfLoader>();
    std::cout << "ResourceManager created." << std::endl;
}

ResourceManager::~ResourceManager() {
    // Cache and Config will be cleaned up by unique_ptr automatically.
    std::cout << "ResourceManager destroyed." << std::endl;
}

bool ResourceManager::Initialize(const std::string& configFilePath) {
    if (!assetConfig_->LoadConfig(configFilePath)) {
        std::cerr << "Error: Failed to load asset configuration from: " << configFilePath << std::endl;
        initialized_ = false;
        return false;
    }
    std::cout << "ResourceManager initialized with config: " << configFilePath << std::endl;
    initialized_ = true;
    return true;
}

// --- Sprite Management ---
std::shared_ptr<SpriteData> ResourceManager::LoadSprite(const std::string& resourceId, const std::string& category) {
    if (!initialized_) {
        std::cerr << "Error: ResourceManager not initialized. Call Initialize() first." << std::endl;
        return nullptr;
    }

    std::shared_ptr<SpriteData> cachedSprite;
    if (assetCache_->TryGetSprite(resourceId, cachedSprite)) {
        // std::cout << "Sprite '" << resourceId << "' found in cache." << std::endl;
        return cachedSprite;
    }

    std::string filePath = assetConfig_->GetAssetPath(category, resourceId);
    if (filePath.empty()) {
        std::cerr << "Error: Sprite path not found for ID '" << resourceId << "' in category '" << category << "'." << std::endl;
        return nullptr;
    }

    // --- Placeholder: Actual sprite loading logic (e.g., from FRM, PNG) ---
    // This would involve reading the file, creating texture, setting up SpriteData fields.
    // For FRM, this would be more complex, potentially involving an FRM parser.
    // For a simple PNG sprite:
    // 1. Load PNG with a library (e.g. SDL_image, stb_image, or a game specific one).
    // 2. Create a Vulkan texture from pixel data (using VulkanResourceAllocator).
    // 3. Store texture handle, dimensions in SpriteData.

    auto newSprite = std::make_shared<SpriteData>();
    newSprite->filePath = filePath; // Store the resolved path
    newSprite->loaded = true; // Assume successful load for this placeholder

    // Example: If SpriteData had a field `int textureId` and `Vec2 dimensions`
    // newSprite->textureId = LoadTextureFromFile(filePath); // Hypothetical
    // newSprite->dimensions = GetTextureDimensions(newSprite->textureId); // Hypothetical
    // --- End Placeholder ---


    if (newSprite->loaded) {
        std::cout << "Sprite loaded from disk: ID '" << resourceId << "', Path '" << filePath << "'" << std::endl;
        assetCache_->StoreSprite(resourceId, newSprite);
        return newSprite;
    } else {
        std::cerr << "Error: Failed to load sprite for ID '" << resourceId << "' from path '" << filePath << "'" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<SpriteData> ResourceManager::GetSprite(const std::string& resourceId) {
    if (!initialized_) return nullptr;
    std::shared_ptr<SpriteData> sprite;
    if (assetCache_->TryGetSprite(resourceId, sprite)) {
        return sprite;
    }
    // Fallback: try to load it if not found in cache (lazy loading)
    // This requires knowing/guessing the category or having a default category.
    // For simplicity, let's assume a "GeneralSprites" category or that resourceId might be structured.
    // std::cout << "Sprite '" << resourceId << "' not in cache. Attempting to load." << std::endl;
    // return LoadSprite(resourceId, "GeneralSprites"); // Example category
    return nullptr; // Or return LoadSprite with a default category if lazy loading is desired
}


// --- 3D Model Management ---
std::shared_ptr<graphics::ModelAsset> ResourceManager::LoadModel(const std::string& resourceId, const std::string& category) {
    if (!initialized_) {
        std::cerr << "Error: ResourceManager not initialized. Call Initialize() first." << std::endl;
        return nullptr;
    }

    std::shared_ptr<graphics::ModelAsset> cachedModel;
    if (assetCache_->TryGetModel(resourceId, cachedModel)) {
        // std::cout << "Model '" << resourceId << "' found in cache." << std::endl;
        return cachedModel;
    }

    std::string filePath = assetConfig_->GetAssetPath(category, resourceId);
    if (filePath.empty()) {
        std::cerr << "Error: Model path not found for ID '" << resourceId << "' in category '" << category << "'." << std::endl;
        return nullptr;
    }

    auto newModel = std::make_shared<graphics::ModelAsset>();
    // GltfLoader could be a member of ResourceManager if used frequently, to avoid re-creation.
    graphics::GltfLoader loader;

    if (loader.Load(filePath, *newModel)) {
        // GltfLoader sets newModel->filePath and newModel->loaded internally.
        std::cout << "Model loaded from disk: ID '" << resourceId << "', Path '" << filePath << "'" << std::endl;
        assetCache_->StoreModel(resourceId, newModel);
        return newModel;
    } else {
        std::cerr << "Error: Failed to load model ID '" << resourceId << "' from path '" << filePath << "'" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<graphics::ModelAsset> ResourceManager::GetModel(const std::string& resourceId) {
    if (!initialized_) return nullptr;
    std::shared_ptr<graphics::ModelAsset> model;
    if (assetCache_->TryGetModel(resourceId, model)) {
        return model;
    }
    // std::cout << "Model '" << resourceId << "' not in cache. Attempting to load." << std::endl;
    // return LoadModel(resourceId, "GeneralModels"); // Example category for lazy load
    return nullptr;
}

// --- AnimationSet Management ---
std::shared_ptr<anim::AnimationSet> ResourceManager::LoadAnimationSet(const std::string& resourceId, const std::string& category) {
     if (!initialized_) {
        std::cerr << "Error: ResourceManager not initialized. Call Initialize() first." << std::endl;
        return nullptr;
    }
    std::shared_ptr<anim::AnimationSet> cachedSet;
    if (assetCache_->TryGetAnimationSet(resourceId, cachedSet)) {
        // std::cout << "AnimationSet '" << resourceId << "' found in cache." << std::endl;
        return cachedSet;
    }

    std::string configPath = assetConfig_->GetAssetPath(category, resourceId);
    if (configPath.empty()) {
        std::cerr << "Error: AnimationSet config path not found for ID '" << resourceId
                  << "' in category '" << category << "'." << std::endl;
        return nullptr;
    }

    // --- Placeholder: Actual AnimationSet loading logic ---
    // This would involve:
    // 1. Determining the type of file at 'configPath' (e.g., a master anim config, an FRM file itself).
    // 2. If it's a config file, parse it to find paths to actual animation files (e.g., FRMs).
    // 3. Load and parse those animation files (e.g., using an FRM parser).
    // 4. Populate an anim::AnimationSet structure with sequences and frames.

    auto newAnimSet = std::make_shared<anim::AnimationSet>();
    newAnimSet->name = resourceId;
    bool loadedSuccessfully = false; // Becomes true if placeholder or actual loading succeeds

    // Example: Manually creating a dummy sequence if configPath was, say, "player_animations_def.json"
    // and we are hardcoding what that means for now.
    if (resourceId == "player_generic_anims") { // Example hardcoded data based on a logical name
        anim::AnimationSequence idleSeq;
        idleSeq.name = "idle_south";
        idleSeq.isLooping = true;

        anim::AnimationFrame frame1;
        frame1.resourceId = 101; // Placeholder FID or texture ID for "player_idle_south_0"
        frame1.duration = 0.5f;
        // frame1.uvOffset, frame1.uvSize would be set if from sprite sheet
        idleSeq.frames.push_back(frame1);

        anim::AnimationFrame frame2;
        frame2.resourceId = 102; // Placeholder FID for "player_idle_south_1"
        frame2.duration = 0.5f;
        idleSeq.frames.push_back(frame2);

        idleSeq.CalculateTotalDuration(); // Important!
        newAnimSet->sequences[idleSeq.name] = idleSeq;
        loadedSuccessfully = true;
    } else {
        // In a real scenario, you would parse 'configPath' here.
        std::cout << "Info: No specific placeholder logic for AnimationSet '" << resourceId
                  << "'. It will be empty unless an actual parser is implemented." << std::endl;
        // For now, we can say it's "loaded" as an empty set if configPath was found.
        // Or, make loadedSuccessfully = false if an empty set is an error.
        // Let's assume an empty set is not an error, but it won't be very useful.
        loadedSuccessfully = true; // Or false, depending on policy for empty/unparsed sets
    }
    // --- End Placeholder ---

    if (loadedSuccessfully) {
        std::cout << "AnimationSet processed/loaded: ID '" << resourceId << "', From primary config: '" << configPath << "'" << std::endl;
        assetCache_->StoreAnimationSet(resourceId, newAnimSet);
        return newAnimSet;
    } else {
        std::cerr << "Error: Failed to load or populate AnimationSet for ID '" << resourceId << "' from config path '" << configPath << "'" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<anim::AnimationSet> ResourceManager::GetAnimationSet(const std::string& resourceId) {
    if (!initialized_) return nullptr;
    std::shared_ptr<anim::AnimationSet> animSet;
    if (assetCache_->TryGetAnimationSet(resourceId, animSet)) {
        return animSet;
    }
    // std::cout << "AnimationSet '" << resourceId << "' not in cache. Attempting to load." << std::endl;
    // return LoadAnimationSet(resourceId, "GeneralAnimations"); // Example category for lazy load
    return nullptr;
}

// --- Texture Management ---
// In ResourceManager.cpp we need to include VulkanResourceAllocator.h to call CreateTextureImage
// However, VulkanResourceAllocator.h includes gVulkan which is in vulkan_render.h
// This creates a messy dependency. For now, assume gVulkan.resourceAllocator_ is accessible.
// A better design would be to pass VulkanResourceAllocator to ResourceManager or make it a singleton.
#include "graphics/vulkan/VulkanResourceAllocator.h" // For CreateTextureImage
extern fallout::VulkanRenderer gVulkan; // HACK: To access resourceAllocator via gVulkan.resourceAllocator_

std::shared_ptr<graphics::TextureAsset> ResourceManager::LoadTexture(const std::string& resourceId, const std::string& category) {
    if (!initialized_) {
        std::cerr << "Error: ResourceManager not initialized for LoadTexture." << std::endl;
        return nullptr;
    }

    std::shared_ptr<graphics::TextureAsset> cachedTexture;
    // Use resourceId as the primary cache key. If filePath is different, that's fine.
    if (assetCache_->TryGetTexture(resourceId, cachedTexture)) {
        return cachedTexture;
    }

    std::string filePath = assetConfig_->GetAssetPath(category, resourceId);
    if (filePath.empty()) {
        // If resourceId is itself a path (e.g. from glTF material), try using it directly.
        // This allows loading textures specified directly by path in models.
        if (resourceId.find('/') != std::string::npos || resourceId.find('\\') != std::string::npos) {
            filePath = resourceId;
        } else {
            std::cerr << "Error: Texture path not found for ID '" << resourceId << "' in category '" << category << "'." << std::endl;
            return nullptr;
        }
    }

    // Second cache check, this time with the resolved file path, in case it was aliased.
    // This is useful if resourceId is a logical name but filePath is what's truly unique for caching raw textures.
    // However, for simplicity, if primary caching is by resourceId, this might be redundant or handled by policy.
    // For now, let's assume cache by resourceId is sufficient. If filePath is the true unique key, cache should use that.
    // Let's adjust to cache by filePath if that's the more unique identifier for raw textures.
    // No, stick to resourceId for cache key, filePath is just for loading if not cached.

    auto newTexture = std::make_shared<graphics::TextureAsset>();
    newTexture->path = filePath;

    if (!gVulkan.resourceAllocator_) {
        std::cerr << "Error: VulkanResourceAllocator not available in ResourceManager (via gVulkan) for LoadTexture." << std::endl;
        return nullptr;
    }

    // CreateTextureImage populates newTexture->imageResource and newTexture->sampler.
    // It also creates the imageView on imageResource.
    if (gVulkan.resourceAllocator_->CreateTextureImage(filePath.c_str(), newTexture->imageResource, newTexture->sampler)) {
        newTexture->imageView = newTexture->imageResource.imageView; // Ensure our TextureAsset also has the view.

        // VRA::CreateTextureImage doesn't directly return width/height.
        // This info is available during stbi_load within CreateTextureImage.
        // For now, TextureAsset.width/height will be 0 unless VRA is modified to fill them
        // or we re-query image properties (which is complex from VkImage handle alone).
        // This is a known gap for this step. Assume 0 for now.
        newTexture->width = 0; // Placeholder
        newTexture->height = 0; // Placeholder

        newTexture->loaded = true;
        std::cout << "Texture loaded: ID '" << resourceId << "', Path '" << filePath << "'" << std::endl;
        assetCache_->StoreTexture(resourceId, newTexture); // Cache using resourceId
        return newTexture;
    } else {
        std::cerr << "Error: Failed to create texture for ID '" << resourceId << "' from path '" << filePath << "'" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<graphics::TextureAsset> ResourceManager::GetTexture(const std::string& resourceId) {
    if (!initialized_) return nullptr;
    std::shared_ptr<graphics::TextureAsset> texture;
    // Try to get by resourceId first (logical name or direct path)
    if (assetCache_->TryGetTexture(resourceId, texture)) {
        return texture;
    }
    // If resourceId might be a path not yet loaded via a logical name,
    // we could try to load it directly, assuming a default category or that path is absolute.
    // This makes GetTexture also a potential loader if the path is directly used.
    // std::cout << "Texture '" << resourceId << "' not in cache by ID. Attempting to load as path." << std::endl;
    // return LoadTexture(resourceId, "DirectPaths"); // "DirectPaths" could be a dummy category for this.
    return nullptr;
}


// --- Clear Cache ---
void ResourceManager::ClearCache() {
    if (assetCache_) {
        assetCache_->ClearAll();
        std::cout << "ResourceManager: Asset cache cleared." << std::endl;
    }
}

} // namespace fallout
