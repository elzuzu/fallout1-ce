#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION // tinygltf needs this for texture loading (if enabled)
#define STB_IMAGE_WRITE_IMPLEMENTATION // tinygltf needs this for texture writing (if enabled)
// Define TINYGLTF_NO_STB_IMAGE if you don't want tinygltf to use stb_image for image loading.
// Define TINYGLTF_NO_STB_IMAGE_WRITE if you don't want tinygltf to use stb_image_write.
// We are not doing texture loading here, but good to be aware.

// This is a virtual path. In a real environment, ensure tiny_gltf.h is accessible.
#include "external/tinygltf/tiny_gltf.h"

#include "GltfLoader.h"
#include <iostream> // For error messages, replace with proper logging

namespace fallout {
namespace graphics {

GltfLoader::GltfLoader() = default;
GltfLoader::~GltfLoader() = default;

bool GltfLoader::Load(const std::string& filePath, ModelAsset& outAsset) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filePath);
    // bool res = loader.LoadBinaryFromFile(&model, &err, &warn, filePath); // For GLB

    if (!warn.empty()) {
        std::cout << "glTF Warning: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "glTF Error: " << err << std::endl;
    }
    if (!res) {
        std::cerr << "Failed to load glTF: " << filePath << std::endl;
        return false;
    }

    outAsset.filePath = filePath;

    // Process materials first, so meshes can reference them by index
    outAsset.materials.reserve(model.materials.size());
    for (size_t i = 0; i < model.materials.size(); ++i) {
        ProcessMaterial(model, static_cast<int>(i), outAsset);
    }

    // Process all meshes referenced by nodes
    // For simplicity, we'll iterate through scenes and their nodes to find meshes.
    // A more robust approach might iterate all model.meshes if not all are instanced in scenes.
    for (const auto& scene : model.scenes) {
        for (int nodeIdx : scene.nodes) {
            if (nodeIdx >= 0 && nodeIdx < model.nodes.size()) {
                 // Recursive node processing (can be complex if full hierarchy is needed)
                 // For this task, let's simplify and just grab meshes directly from nodes.
                 // ProcessNode(model, model.nodes[nodeIdx], outAsset, -1);
            }
        }
    }
    // Simpler: iterate all nodes that have meshes.
    // This might create duplicates if nodes instantiate the same mesh multiple times with different transforms.
    // For now, we'll just load unique meshes.
    outAsset.meshes.resize(model.meshes.size()); // Pre-allocate assuming one MeshData per glTF mesh
    bool anyMeshLoaded = false;
    for(size_t i = 0; i < model.meshes.size(); ++i) {
        // We are not using ProcessMesh directly here to avoid node transforms for now.
        // Instead, we create one MeshData per glTF mesh.
        const tinygltf::Mesh& gltfMesh = model.meshes[i];
        if (!gltfMesh.primitives.empty()) {
            // For simplicity, we combine all primitives of a glTF mesh into one MeshData.
            // A more accurate representation would have MeshData per primitive.
            MeshData combinedMeshData;
            combinedMeshData.materialIndex = -1; // Default

            for (const auto& primitive : gltfMesh.primitives) {
                MeshData primitiveMeshData; // Temporary for this primitive
                ProcessPrimitive(model, primitive, gltfMesh.name, primitiveMeshData, outAsset);

                // Combine vertex/index data
                // Adjust indices from primitiveMeshData before adding
                uint32_t indexOffset = static_cast<uint32_t>(combinedMeshData.vertices.size());
                for(uint32_t idx : primitiveMeshData.indices) {
                    combinedMeshData.indices.push_back(idx + indexOffset);
                }
                combinedMeshData.vertices.insert(combinedMeshData.vertices.end(), primitiveMeshData.vertices.begin(), primitiveMeshData.vertices.end());

                if(combinedMeshData.materialIndex == -1 && primitiveMeshData.materialIndex != -1) {
                     combinedMeshData.materialIndex = primitiveMeshData.materialIndex;
                } else if (primitiveMeshData.materialIndex != -1 && combinedMeshData.materialIndex != primitiveMeshData.materialIndex) {
                    // Primitives within the same glTF mesh use different materials.
                    // Our current MeshData supports one material. This needs a more complex structure
                    // or splitting the mesh. For now, log a warning.
                    std::cerr << "Warning: Mesh '" << gltfMesh.name << "' has primitives with different materials. "
                              << "Only the material of the first primitive will be used for the combined mesh." << std::endl;
                }
            }
            if (!combinedMeshData.vertices.empty()) {
                 outAsset.meshes[i] = combinedMeshData; // Store the combined mesh
                 anyMeshLoaded = true;
            }
        }
    }
     // Clean up meshes that weren't loaded (if we pre-allocated and some glTF meshes had no primitives)
    if(anyMeshLoaded){
        outAsset.meshes.erase(std::remove_if(outAsset.meshes.begin(), outAsset.meshes.end(),
                                            [](const MeshData& m) { return m.vertices.empty(); }),
                            outAsset.meshes.end());
    }


    outAsset.loaded = anyMeshLoaded || !outAsset.materials.empty();
    return outAsset.loaded;
}

void GltfLoader::ProcessMaterial(const tinygltf::Model& model, int materialIdx, ModelAsset& outAsset) {
    if (materialIdx < 0 || materialIdx >= model.materials.size()) {
        return;
    }
    const tinygltf::Material& gltfMat = model.materials[materialIdx];
    MaterialInfo materialInfo;
    materialInfo.name = gltfMat.name;

    materialInfo.baseColorFactor.x = static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[0]);
    materialInfo.baseColorFactor.y = static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[1]);
    materialInfo.baseColorFactor.z = static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[2]);
    materialInfo.baseColorFactor.w = static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[3]);

    if (gltfMat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
        const tinygltf::Texture& tex = model.textures[gltfMat.pbrMetallicRoughness.baseColorTexture.index];
        if (tex.source >= 0 && tex.source < model.images.size()) {
            materialInfo.baseColorTexturePath = model.images[tex.source].uri;
            // Note: If the image is embedded or uses a data URI, this path might not be directly usable.
            // Full texture loading would require decoding this URI or data buffer.
        }
    }
    materialInfo.doubleSided = gltfMat.doubleSided;
    // TODO: Load other PBR parameters (metallic, roughness, textures) as needed

    outAsset.materials.push_back(materialInfo);
}


void GltfLoader::ProcessPrimitive(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
                                   const std::string& meshName, MeshData& outMeshData, ModelAsset& outAsset) {
    if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
        std::cerr << "Warning: Skipping non-triangle primitive in mesh '" << meshName << "'" << std::endl;
        return;
    }

    // Indices
    if (primitive.indices >= 0) {
        ExtractIndexData(model, primitive.indices, outMeshData.indices);
    } else {
        // Non-indexed geometry. We'd need to generate trivial indices if our renderer requires them.
        // Or handle non-indexed drawing. For now, we assume indexed or auto-generate.
        // If no indices, but there are vertices, create linear indices
        auto posIt = primitive.attributes.find("POSITION");
        if (posIt != primitive.attributes.end()) {
            const tinygltf::Accessor& posAccessor = model.accessors[posIt->second];
             outMeshData.indices.resize(posAccessor.count);
             for(size_t i = 0; i < posAccessor.count; ++i) outMeshData.indices[i] = static_cast<uint32_t>(i);
        } else {
            std::cerr << "Warning: Primitive in mesh '" << meshName << "' has no POSITION attribute and no indices. Skipping." << std::endl;
            return;
        }
    }

    // Vertices
    size_t vertexCount = 0;
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<Vec4> colors;

    for (const auto& attribute : primitive.attributes) {
        const std::string& attrName = attribute.first;
        int accessorIndex = attribute.second;

        if (attrName == "POSITION") {
            ExtractVertexData<Vec3>(model, accessorIndex, positions);
            vertexCount = std::max(vertexCount, positions.size());
        } else if (attrName == "NORMAL") {
            ExtractVertexData<Vec3>(model, accessorIndex, normals);
        } else if (attrName == "TEXCOORD_0") {
            ExtractVertexData<Vec2>(model, accessorIndex, uvs);
        } else if (attrName == "COLOR_0") { // Vertex colors
             const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
            if (accessor.type == TINYGLTF_TYPE_VEC3) {
                std::vector<Vec3> colors_v3;
                ExtractVertexData<Vec3>(model, accessorIndex, colors_v3);
                colors.resize(colors_v3.size());
                for(size_t i=0; i<colors_v3.size(); ++i) colors[i] = {colors_v3[i].x, colors_v3[i].y, colors_v3[i].z, 1.0f};
            } else if (accessor.type == TINYGLTF_TYPE_VEC4) {
                 ExtractVertexData<Vec4>(model, accessorIndex, colors);
            }
        }
    }

    if (positions.empty()) {
         std::cerr << "Warning: Primitive in mesh '" << meshName << "' has no POSITION attribute. Skipping." << std::endl;
        return;
    }
    vertexCount = positions.size(); // Position is mandatory

    outMeshData.vertices.resize(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i) {
        outMeshData.vertices[i].position = positions[i];
        if (i < normals.size()) outMeshData.vertices[i].normal = normals[i];
        else outMeshData.vertices[i].normal = {0,0,1}; // Default normal
        if (i < uvs.size()) outMeshData.vertices[i].uv = uvs[i];
        else outMeshData.vertices[i].uv = {0,0}; // Default UV
        if (i < colors.size()) outMeshData.vertices[i].color = colors[i];
        else outMeshData.vertices[i].color = {1,1,1,1}; // Default white
    }

    if (primitive.material >= 0 && primitive.material < outAsset.materials.size()) {
        outMeshData.materialIndex = primitive.material;
    } else if (primitive.material >= 0) {
        // This case should ideally not happen if materials are processed first and are contiguous
        std::cerr << "Warning: Primitive references material index " << primitive.material
                  << " but only " << outAsset.materials.size() << " materials loaded." << std::endl;
        outMeshData.materialIndex = -1;
    } else {
        outMeshData.materialIndex = -1; // No material
    }
}


template<typename T_VertexType>
void GltfLoader::ExtractVertexData(const tinygltf::Model& model, int accessorIndex, std::vector<T_VertexType>& outVector) {
    if (accessorIndex < 0 || accessorIndex >= model.accessors.size()) return;
    const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

    outVector.resize(accessor.count);
    const unsigned char* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
    size_t elementSize = tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);

    // Ensure T_VertexType matches the data in glTF or perform conversion
    // This simple version assumes direct copy is possible.
    // A robust version would check accessor.componentType and accessor.type
    // and handle conversions (e.g. short to float, vec2 to vec3 by padding).
    if (sizeof(T_VertexType) > elementSize && accessor.normalized) {
        // Handle normalized integers being converted to floats (not fully implemented here)
        // For example, if T_VertexType is float and glTF stores normalized ubyte.
    }


    for (size_t i = 0; i < accessor.count; ++i) {
        memcpy(&outVector[i], dataPtr + i * (bufferView.byteStride ? bufferView.byteStride : elementSize), elementSize);
        // If accessor.normalized is true for integer types, they should be converted to float [0,1] or [-1,1]
    }
}

void GltfLoader::ExtractIndexData(const tinygltf::Model& model, int accessorIndex, std::vector<uint32_t>& outIndices) {
    if (accessorIndex < 0 || accessorIndex >= model.accessors.size()) return;
    const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

    outIndices.resize(accessor.count);
    const unsigned char* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
    size_t componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    size_t elementSize = componentSize * tinygltf::GetNumComponentsInType(accessor.type); // Should be SCALAR for indices

    for (size_t i = 0; i < accessor.count; ++i) {
        uint32_t index = 0;
        const unsigned char* itemPtr = dataPtr + i * (bufferView.byteStride ? bufferView.byteStride : elementSize);
        switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                index = static_cast<uint32_t>(*reinterpret_cast<const uint8_t*>(itemPtr));
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                index = static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(itemPtr));
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                index = *reinterpret_cast<const uint32_t*>(itemPtr);
                break;
            default:
                // Should not happen for valid glTF indices
                std::cerr << "Error: Unsupported index component type: " << accessor.componentType << std::endl;
                outIndices.clear();
                return;
        }
        outIndices[i] = index;
    }
}
// ProcessNode and full scene hierarchy support would go here if needed.
// For now, we are loading meshes directly.

} // namespace graphics
} // namespace fallout
