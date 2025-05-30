#ifndef FALLOUT_GRAPHICS_RENDERABLE_TYPES_H_
#define FALLOUT_GRAPHICS_RENDERABLE_TYPES_H_

#include <vector>
#include <string>

// Define for simple math types, or include from an existing math library if available
// For now, simple structs. In a real engine, use something like GLM.
struct Vec2 { float x, y; };
struct Vec3 { float x, y, z; };
struct Vec4 { float x, y, z, w; };

namespace fallout {
namespace graphics {

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    Vec4 color; // Vertex color, if available
};

struct MaterialInfo {
    std::string name;
    Vec4 baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
    std::string baseColorTexturePath;
    // Other PBR properties can be added here:
    // float metallicFactor = 0.0f;
    // float roughnessFactor = 1.0f;
    // std::string metallicRoughnessTexturePath;
    // std::string normalTexturePath;
    // std::string occlusionTexturePath;
    // std::string emissiveTexturePath;
    // Vec3 emissiveFactor = {0.0f, 0.0f, 0.0f};
    bool doubleSided = false;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices; // Assuming uint32_t for indices
    int materialIndex = -1;       // Index into a model's material list

    // Bounding box (optional, but good for culling/physics)
    // Vec3 minBounds, maxBounds;
};

struct ModelNode { // For scene hierarchy, simplified for now
    std::string name;
    std::vector<int> meshIndices; // Indices into the Model's mesh list
    // Mat4 transform; // Local transform
    // std::vector<int> children; // Indices to child nodes
};

struct ModelAsset {
    std::string filePath; // Path from which it was loaded
    std::vector<MeshData> meshes;
    std::vector<MaterialInfo> materials;
    std::vector<ModelNode> nodes; // Simplified hierarchy, or just a flat list of meshes
    // TODO: Add animations, skins etc. later
    bool loaded = false;
};


// Data for a loaded texture (GPU resources)
#include "vulkan/vulkan.h" // For VkSampler, VkImageView - or forward declare if minimal
#include "VulkanResourceAllocator.h" // For AllocatedImage - this creates a slight dependency issue if VRA includes RenderableTypes.
                                   // Better to forward declare VkSampler, VkImageView, and move AllocatedImage to its own header if not already.
                                   // For now, assume includes are okay for the tool.

struct TextureAsset {
    std::string path;
    vk::AllocatedImage imageResource; // Contains VkImage, VmaAllocation
                                      // ImageView is now part of AllocatedImage in VRA, but might be separate
    VkImageView imageView = VK_NULL_HANDLE; // Explicitly store here, VRA's AllocatedImage.imageView might not be set by CreateImage
    VkSampler sampler = VK_NULL_HANDLE;
    bool loaded = false;
    uint32_t width = 0;
    uint32_t height = 0;
};


} // namespace graphics
} // namespace fallout

#endif // FALLOUT_GRAPHICS_RENDERABLE_TYPES_H_
