# Guide d'implémentation Vulkan pour Fallout1-CE

## Vue d'ensemble du pipeline Vulkan

Voici l'architecture du pipeline graphique Vulkan d'après la documentation officielle :

```
INPUT DATA → VERTEX SHADER → TESSELLATION → GEOMETRY → RASTERIZATION → FRAGMENT SHADER → COLOR BLEND → FRAMEBUFFER
    ↓              ↓             ↓           ↓             ↓              ↓              ↓
 Vertices      Transform      Subdivide   Add/Remove    Pixel Gen     Color Calc     Final Output
 Indices       Positions      Geometry    Primitives    Fragments     Texturing      Blending
 Attributes    Lighting                                               Lighting
```

**Légende des étapes** :
- 🟡 **Programmables** : Vertex Shader, Tessellation, Geometry, Fragment Shader
- 🟢 **Fixes** : Input Assembly, Rasterization, Color Blend, Output Merger

## 3. Support des modèles glTF 2.0

### 3.1 Parseur glTF

**Librairies recommandées** :
- **cgltf** : Parser C léger et rapide
- **tinygltf** : Plus complet, support des extensions

**Structure de données** :
```c
typedef struct GLTFScene {
    // Geometry
    Buffer* vertex_buffers;
    Buffer* index_buffers;
    uint32_t mesh_count;
    
    // Materials  
    Material* materials;
    Texture* textures;
    Sampler* samplers;
    
    // Animation
    Animation* animations;
    Node* nodes;  // Hiérarchie des transforms
    
    // Skinning
    Skin* skins;
    Buffer* joint_matrices;
} GLTFScene;
```

### 3.2 Chargement des assets

**Pipeline de chargement** :
1. **Parse JSON** : Structure du fichier glTF
2. **Load Buffers** : Données binaires (vertex, indices, textures)
3. **Create GPU Resources** : Upload vers VRAM
4. **Build Scene Graph** : Hiérarchie des nodes
5. **Setup Animations** : Timeline et interpolation

**Optimisations** :
- Chargement asynchrone en arrière-plan
- Compression des textures (BC7, ASTC)
- Merge des vertex buffers compatibles
- Instancing pour les objets répétés

### 3.3 Materials PBR

**Support des materials glTF** :
- **Base Color** : Albedo + alpha
- **Metallic-Roughness** : Workflow PBR standard
- **Normal Maps** : Detail des surfaces
- **Occlusion** : AO pré-calculé
- **Emissive** : Matériaux lumineux

**Extensions glTF à supporter** :
- `KHR_materials_unlit` : Pour les sprites/UI
- `KHR_texture_transform` : UV mapping avancé
- `KHR_materials_ior` : Réflexions physiques

### 3.4 Animation et skinning

**Système d'animation** :
```c
typedef struct Animation {
    float duration;
    uint32_t channel_count;
    AnimationChannel* channels;
    
    // Runtime
    float current_time;
    bool is_playing;
    bool is_looping;
} Animation;

typedef struct AnimationChannel {
    NodeID target_node;
    AnimationProperty property; // Translation, Rotation, Scale
    Keyframe* keyframes;
    InterpolationType interpolation;
} AnimationChannel;
```

**Skinning sur GPU** :
- Matrices de joints dans un UBO
- Vertex shader avec blend weights
- Support jusqu'à 4 joints par vertex (limitation glTF)


## Nouvelles améliorations Vulkan
- Recréation robuste du swapchain avec gestion de `VK_ERROR_DEVICE_LOST`.
- Conversion palette 8 bits en RGBA via LUT 1D sur GPU.
- Décodage vidéo YUV420 en RGBA effectué par un pipeline compute utilisant `KHR_sampler_ycbcr_conversion`.
- Cache de pipeline persistant sauvegardé dans `pipeline_cache.bin`.
- Allocation dUBO persistante exposée via `VK_KHR_buffer_device_address`.

