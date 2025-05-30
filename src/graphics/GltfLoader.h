#ifndef FALLOUT_GRAPHICS_GLTF_LOADER_H_
#define FALLOUT_GRAPHICS_GLTF_LOADER_H_

#include "graphics/RenderableTypes.h"
#include <string>

// Forward declare TinyGLTF types if possible, to avoid including tiny_gltf.h in this header.
// This is often tricky if you need to pass TinyGLTF model parts around.
// For simplicity here, we might include it, or rely on GltfLoader.cpp to handle all tinygltf specifics.
namespace tinygltf {
    class Model;
    struct Node;
    struct Mesh;
    struct Primitive;
}

namespace fallout {
namespace graphics {

class GltfLoader {
public:
    GltfLoader();
    ~GltfLoader();

    // Loads a glTF model from the given file path.
    // Populates the ModelAsset structure with mesh, material, and node data.
    // Returns true on success, false on failure.
    bool Load(const std::string& filePath, ModelAsset& outAsset);

private:
    // Helper functions to process different parts of the glTF model
    void ProcessNode(const tinygltf::Model& model, const tinygltf::Node& node, ModelAsset& outAsset, int parentNodeIndex);
    void ProcessMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const std::string& meshName, ModelAsset& outAsset);
    void ProcessPrimitive(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const std::string& meshName, MeshData& outMeshData, ModelAsset& outAsset);
    void ProcessMaterial(const tinygltf::Model& model, int materialIdx, ModelAsset& outAsset);

    // Helper to extract vertex attribute data
    template<typename T_VertexType>
    void ExtractVertexData(const tinygltf::Model& model, int accessorIndex, std::vector<T_VertexType>& outVector);
    void ExtractIndexData(const tinygltf::Model& model, int accessorIndex, std::vector<uint32_t>& outIndices);
};

} // namespace graphics
} // namespace fallout

#endif // FALLOUT_GRAPHICS_GLTF_LOADER_H_
