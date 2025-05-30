#ifndef TINYGLTF_STUB_H
#define TINYGLTF_STUB_H
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <cstring>

namespace tinygltf {

// Minimal structures to satisfy the loader implementation.
struct Buffer { std::vector<unsigned char> data; };
struct BufferView { int buffer = 0; size_t byteOffset = 0; size_t byteLength = 0; size_t byteStride = 0; };

struct Accessor {
  int bufferView = 0;
  size_t byteOffset = 0;
  size_t count = 0;
  int componentType = 0;
  int type = 0;
  bool normalized = false;
};

struct Image { std::string uri; };
struct Texture { int source = -1; };

struct PbrMetallicRoughness {
  double baseColorFactor[4] = {1.0,1.0,1.0,1.0};
  int baseColorTexture{-1};
  double metallicFactor{1.0};
  double roughnessFactor{1.0};
  int metallicRoughnessTexture{-1};
};

struct NormalTextureInfo { int index = -1; };
struct OcclusionTextureInfo { int index = -1; };
struct EmissiveTextureInfo { int index = -1; };

struct Material {
  std::string name;
  PbrMetallicRoughness pbrMetallicRoughness;
  NormalTextureInfo normalTexture;
  OcclusionTextureInfo occlusionTexture;
  EmissiveTextureInfo emissiveTexture;
  double emissiveFactor[3] = {0.0,0.0,0.0};
  bool doubleSided = false;
};

struct Primitive {
  std::map<std::string, int> attributes;
  int indices = -1;
  int material = -1;
  int mode = 4; // triangles
};

struct Mesh { std::string name; std::vector<Primitive> primitives; };

struct Node {
  std::string name;
  int mesh = -1;
  std::vector<int> children;
  std::vector<double> translation;
  std::vector<double> rotation;
  std::vector<double> scale;
};

struct AnimationSampler {
  std::vector<double> inputTimes;
  std::vector<std::vector<double>> outputVec3; // translate only in stub
};
struct AnimationChannel {
  int sampler = -1;
  int target_node = -1;
};
struct Animation {
  std::string name;
  std::vector<AnimationSampler> samplers;
  std::vector<AnimationChannel> channels;
};

struct Scene { std::vector<int> nodes; };

struct Model {
  std::vector<Buffer> buffers;
  std::vector<BufferView> bufferViews;
  std::vector<Accessor> accessors;
  std::vector<Image> images;
  std::vector<Texture> textures;
  std::vector<Material> materials;
  std::vector<Mesh> meshes;
  std::vector<Node> nodes;
  std::vector<Scene> scenes;
  std::vector<Animation> animations;
};

class TinyGLTF {
public:
  bool LoadASCIIFromFile(Model* model, std::string*, std::string*, const std::string& filename) {
    if (!model) return false;
    std::ifstream ifs(filename);
    if (!ifs.is_open()) return false;
    std::string token;
    while (ifs >> token) {
      if (token == "MATERIAL") {
        Material m; ifs >> m.name;
        for (int i=0;i<4;i++) ifs >> m.pbrMetallicRoughness.baseColorFactor[i];
        ifs >> m.pbrMetallicRoughness.metallicFactor;
        ifs >> m.pbrMetallicRoughness.roughnessFactor;
        model->materials.push_back(m);
      } else if (token == "MESH") {
        Mesh mesh; size_t vcount, icount; ifs >> mesh.name >> vcount >> icount;
        Primitive prim; prim.attributes["POSITION"] = 0; prim.indices = 1; mesh.primitives.push_back(prim);
        model->meshes.push_back(mesh);
        model->buffers.resize(2);
        model->bufferViews.resize(2);
        model->accessors.resize(2);
        model->buffers[0].data.resize(vcount * sizeof(float) * 12);
        model->buffers[1].data.resize(icount * sizeof(uint32_t));
        model->accessors[0].count = vcount;
        model->accessors[0].componentType = 5126;
        model->accessors[0].type = 2;
        model->accessors[1].count = icount;
        model->accessors[1].componentType = 5125;
        model->accessors[1].type = 0;
        for(size_t i=0;i<vcount;i++) {
          float vals[12];
          for(int j=0;j<12;j++) ifs >> vals[j];
          std::memcpy(&model->buffers[0].data[i*sizeof(float)*12], vals, sizeof(float)*12);
        }
        for(size_t i=0;i<icount;i++) {
          uint32_t idx; ifs >> idx;
          std::memcpy(&model->buffers[1].data[i*sizeof(uint32_t)], &idx, sizeof(uint32_t));
        }
      } else if (token == "NODE") {
        Node node; ifs >> node.name >> node.mesh; int parent; ifs >> parent;
        node.translation.resize(3); for(int i=0;i<3;i++) ifs >> node.translation[i];
        if (parent >=0 && parent < (int)model->nodes.size()) {
          model->nodes[parent].children.push_back((int)model->nodes.size());
        }
        model->nodes.push_back(node);
      } else if (token == "ANIM") {
        Animation anim; ifs >> anim.name; double duration; ifs >> duration; (void)duration;
        model->animations.push_back(anim);
      } else if (token == "KEY") {
        int animIndex,nodeIndex; double time; ifs >> animIndex >> nodeIndex >> time;
        double tx,ty,tz; ifs >> tx >> ty >> tz;
        if (animIndex < 0 || animIndex >= (int)model->animations.size()) continue;
        Animation& anim = model->animations[animIndex];
        AnimationSampler samp; samp.inputTimes.push_back(time); samp.outputVec3.push_back({tx,ty,tz});
        anim.samplers.push_back(samp);
        AnimationChannel chan; chan.sampler = (int)anim.samplers.size()-1; chan.target_node = nodeIndex; anim.channels.push_back(chan);
      }
    }
    return true;
  }
};

// Utility helpers
static inline size_t GetComponentSizeInBytes(int componentType) {
  switch(componentType) {
    case 5120: // BYTE
    case 5121: // UNSIGNED_BYTE
      return 1;
    case 5122: // SHORT
    case 5123: // UNSIGNED_SHORT
      return 2;
    case 5125: // UNSIGNED_INT
    case 5126: // FLOAT
      return 4;
    default: return 0;
  }
}

static inline size_t GetNumComponentsInType(int type) {
  switch(type) {
    case 0: return 1; // SCALAR
    case 1: return 2; // VEC2
    case 2: return 3; // VEC3
    case 3: return 4; // VEC4
    default: return 0;
  }
}

} // namespace tinygltf

#endif
