/*** Context — Hybrid Asset Pipeline for Fallout CE ***/
#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include "render/fallout_memory_manager.h"
namespace fallout {

/*** Legacy Fallout Asset Support ***/
#pragma pack(push, 1)
struct FalloutFRMHeader {
    uint32_t version;           // Always 4
    uint16_t framesPerSecond;   
    uint16_t actionFrame;       // Frame that contains the action
    uint16_t framesCount;       // Number of frames
    uint16_t directions[6];     // Frames per direction (0-5)
    int16_t  shiftX[6];         // X offset for each direction
    int16_t  shiftY[6];         // Y offset for each direction
    uint32_t dataOffset[6];     // Offset to frame data for each direction
    uint32_t frameDataSize;     // Size of frame data
};

struct FalloutFRMFrame {
    uint16_t width;
    uint16_t height;
    uint32_t dataSize;
    int16_t  offsetX;
    int16_t  offsetY;
    // Followed by pixel data
};

struct FalloutPALHeader {
    uint8_t colors[256][3];     // RGB palette (256 colors)
};
#pragma pack(pop)

/*** Modern glTF Support ***/
struct glTFPrimitive {
    uint64_t vertexBufferId = 0;
    uint64_t indexBufferId = 0;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
    uint32_t materialIndex = 0;
    
    // Bounding box
    glm::vec3 minBounds = glm::vec3(0.0f);
    glm::vec3 maxBounds = glm::vec3(0.0f);
};

struct glTFMaterial {
    std::string name;
    
    // PBR workflow
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    glm::vec3 emissiveFactor = glm::vec3(0.0f);
    float normalScale = 1.0f;
    float occlusionStrength = 1.0f;
    
    // Texture indices
    uint64_t baseColorTextureId = 0;
    uint64_t metallicRoughnessTextureId = 0;
    uint64_t normalTextureId = 0;
    uint64_t occlusionTextureId = 0;
    uint64_t emissiveTextureId = 0;
    
    // Alpha mode
    enum class AlphaMode { OPAQUE, MASK, BLEND };
    AlphaMode alphaMode = AlphaMode::OPAQUE;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;
};

struct glTFNode {
    std::string name;
    glm::mat4 transform = glm::mat4(1.0f);
    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    
    std::vector<uint32_t> children;
    std::vector<uint32_t> meshes;
    
    // Skinning data
    int32_t skin = -1;
    std::vector<glm::mat4> inverseBindMatrices;
    std::vector<uint32_t> joints;
};

struct glTFAnimation {
    std::string name;
    float duration = 0.0f;
    
    struct Channel {
        uint32_t nodeIndex;
        enum class Path { TRANSLATION, ROTATION, SCALE, WEIGHTS };
        Path path;
        
        struct Sampler {
            enum class Interpolation { LINEAR, STEP, CUBICSPLINE };
            Interpolation interpolation = Interpolation::LINEAR;
            std::vector<float> input;   // Time values
            std::vector<glm::vec4> output; // Transform values
        } sampler;
    };
    
    std::vector<Channel> channels;
};

/*** Unified Asset Types ***/
enum class AssetType {
    LEGACY_SPRITE_2D,    // .FRM files
    MODERN_MESH_3D,      // glTF meshes
    TEXTURE_2D,          // Various texture formats
    ANIMATION_DATA,      // Animation clips
    MATERIAL_DEFINITION, // PBR materials
    AUDIO_CLIP,          // Sound files
    SCENE_DATA           // Complete scenes
};

struct AssetMetadata {
    AssetType type;
    std::string name;
    std::string originalPath;
    std::string fallbackPath;   // Legacy asset fallback
    uint64_t fileSize = 0;
    uint64_t loadTime = 0;      // microseconds
    bool isLoaded = false;
    bool hasLegacyFallback = false;
    
    // Version info for hot-reloading
    uint64_t lastModified = 0;
    uint32_t version = 1;
};

/*** Hybrid Asset Manager ***/
class FalloutAssetManager {
private:
    FalloutMemoryManager* memoryManager = nullptr;
    VkDevice device = VK_NULL_HANDLE;
    
    // Asset registries
    std::unordered_map<std::string, AssetMetadata> assetRegistry;
    std::unordered_map<std::string, uint64_t> textureRegistry;
    std::unordered_map<std::string, glTFMaterial> materialRegistry;
    std::unordered_map<std::string, std::vector<glTFPrimitive>> meshRegistry;
    std::unordered_map<std::string, glTFAnimation> animationRegistry;
    
    // Legacy Fallout data
    std::unordered_map<std::string, FalloutPALHeader> paletteRegistry;
    std::unordered_map<std::string, std::vector<uint64_t>> spriteRegistry; // FRM textures
    
    // Asset search paths
    std::vector<std::string> assetPaths = {
        "./assets/modern/",     // Modern 3D assets
        "./assets/legacy/",     // Original Fallout assets
        "./data/"              // Original game data
    };
    
    // Loading statistics
    struct LoadingStats {
        uint32_t totalAssetsLoaded = 0;
        uint32_t legacyAssetsLoaded = 0;
        uint32_t modernAssetsLoaded = 0;
        uint64_t totalLoadTime = 0;
        uint64_t totalMemoryUsed = 0;
    } stats;
    
    // Fallback system
    bool enableLegacyFallback = true;
    bool preferModernAssets = true;
    
public:
    /*** Initialization ***/
    bool initialize(FalloutMemoryManager* memMgr, VkDevice dev);
    
    /*** Asset Loading - Automatic Modern/Legacy Detection ***/
    bool loadAsset(const std::string& assetName, AssetType expectedType = AssetType::LEGACY_SPRITE_2D);
    
    /*** Legacy Asset Loading (.FRM files) ***/
    bool loadLegacySprite(const std::string& spriteName, const std::string& filePath);
    
    /*** Modern Asset Loading (glTF) ***/
    bool loadModernMesh(const std::string& meshName, const std::string& filePath);
    
    /*** Hybrid Rendering Interface ***/
    struct RenderableAsset {
        AssetType type;
        std::string name;
        
        union {
            struct {  // Legacy 2D sprite
                std::vector<uint64_t>* frameTextures;
                uint32_t currentFrame;
                uint32_t direction;
            } sprite;
            
            struct {  // Modern 3D mesh
                std::vector<glTFPrimitive>* primitives;
                glm::mat4 transform;
                uint32_t materialIndex;
            } mesh;
        };
    };
    
    std::optional<RenderableAsset> getRenderableAsset(const std::string& assetName);
    
    /*** Asset Management ***/
    bool isAssetLoaded(const std::string& assetName) const;
    AssetType getAssetType(const std::string& assetName) const;
    void unloadAsset(const std::string& assetName);
    LoadingStats getLoadingStats() const;
    void generateAssetReport(const std::string& filename) const;
    
private:
    // Helper methods for asset loading and management
    bool loadAssetRegistry();
    bool loadLegacyPalettes();
    bool findModernAsset(const std::string& name, std::string& path, AssetType& type);
    bool findLegacyAsset(const std::string& name, std::string& path, AssetType& type);
    bool loadModernAsset(const std::string& name, const std::string& path, AssetType type);
    bool loadLegacyAsset(const std::string& name, const std::string& path, AssetType type);
    bool convertPalettizedToRGBA(const std::vector<uint8_t>& palData, uint32_t width, uint32_t height, std::vector<uint8_t>& rgbaData);
    uint64_t createTextureFromData(const void* data, uint32_t width, uint32_t height, VkFormat format, const std::string& debugName);
    void recordLoadingStats(const std::string& assetName, bool isLegacy, uint64_t startTime);
    uint64_t getCurrentTimeMicroseconds() const;
    std::string assetTypeToString(AssetType type) const;
    std::string formatBytes(uint64_t bytes) const;
    
    // glTF helper methods (would integrate with actual library)
    struct glTFModel; // Forward declaration
    glTFModel loadglTFFile(const std::string& path);
    std::vector<Vertex> extractVertexData(const glTFPrimitive& primitive);
    std::vector<uint32_t> extractIndexData(const glTFPrimitive& primitive);
    uint64_t loadTextureFromglTF(const glTFModel& model, int textureIndex, const std::string& name);
    std::vector<float> extractTimeValues(const glTFModel& model, int accessorIndex);
    std::vector<glm::vec4> extractTransformValues(const glTFModel& model, int accessorIndex);
    float calculateAnimationDuration(const std::vector<glTFAnimation::Channel>& channels);
    void calculateBoundingBox(const std::vector<Vertex>& vertices, glm::vec3& min, glm::vec3& max);
};
} // namespace fallout
