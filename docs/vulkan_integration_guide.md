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

## 1. Compléter le moteur Vulkan (Pipeline 3D complet)

### 1.1 Architecture actuelle et extensions nécessaires

**État actuel** : Votre projet a déjà un renderer Vulkan expérimental activé via `RENDER_BACKEND=VULKAN`.

**Extensions Vulkan à vérifier** :
```c
// Dans votre VkInstance
VK_KHR_surface
VK_KHR_win32_surface  // Pour Windows
VK_EXT_debug_utils    // Pour le debug

// Dans votre VkDevice  
VK_KHR_swapchain
VK_EXT_graphics_pipeline_library  // Optionnel pour performance
```

### 1.2 Structure du pipeline 3D complet

**Composants principaux à implémenter** :

#### A. Command Pool & Command Buffers
```c
typedef struct VulkanCommandManager {
    VkCommandPool graphics_pool;
    VkCommandPool transfer_pool;
    VkCommandBuffer* primary_buffers;
    VkCommandBuffer* secondary_buffers;
    uint32_t frame_count;
} VulkanCommandManager;
```

**Étapes d'implémentation** :
1. Créer des command pools séparés pour graphics et transfert
2. Allouer des command buffers pour chaque frame en flight (double/triple buffering)
3. Implémenter la réinitialisation des command buffers par frame

#### B. Pipeline States pour différents usages

**Pipelines nécessaires** :
1. **Sprite Pipeline** (actuel) - pour compatibility fallback
2. **Model Pipeline** - pour les modèles 3D glTF
3. **UI Pipeline** - pour l'interface Fallout
4. **Post-Process Pipeline** - pour les effets

**Structure de base** :
```c
typedef struct VulkanPipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
    VkDescriptorSetLayout desc_layout;
    VkRenderPass render_pass;
    shader_type_t type;
} VulkanPipeline;
```

#### C. Gestion des ressources et descriptors

**Système de descriptor sets** :
- **Set 0** : Données globales (matrices view/projection, lighting)
- **Set 1** : Données par objet (matrice model, material ID)
- **Set 2** : Textures et samplers
- **Set 3** : Données spécialisées (animation, morphing)

### 1.3 Shaders essentiels à créer

#### Vertex Shader (vertex.vert)
```glsl
#version 450

// Set 0 - Global
layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
    vec3 camera_pos;
    float time;
} global;

// Set 1 - Per Object  
layout(set = 1, binding = 0) uniform ObjectUBO {
    mat4 model;
    uint material_id;
} object;

// Input attributes
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec4 joint_indices;
layout(location = 4) in vec4 joint_weights;

// Output to fragment shader
layout(location = 0) out vec3 world_pos;
layout(location = 1) out vec3 world_normal;
layout(location = 2) out vec2 uv;

void main() {
    // Animation/skinning logic ici
    vec3 animated_pos = position; // À compléter avec skinning
    vec3 animated_normal = normal;
    
    world_pos = (object.model * vec4(animated_pos, 1.0)).xyz;
    world_normal = normalize((object.model * vec4(animated_normal, 0.0)).xyz);
    uv = texcoord;
    
    gl_Position = global.proj * global.view * vec4(world_pos, 1.0);
}
```

#### Fragment Shader (fragment.frag)
```glsl
#version 450

// Set 2 - Textures
layout(set = 2, binding = 0) uniform sampler2D albedo_tex;
layout(set = 2, binding = 1) uniform sampler2D normal_tex;
layout(set = 2, binding = 2) uniform sampler2D metallic_roughness_tex;

// Input from vertex shader
layout(location = 0) in vec3 world_pos;
layout(location = 1) in vec3 world_normal;
layout(location = 2) in vec2 uv;

// Output
layout(location = 0) out vec4 color;

void main() {
    // PBR shading simplifié pour Fallout
    vec3 albedo = texture(albedo_tex, uv).rgb;
    vec3 normal = normalize(world_normal); // À compléter avec normal mapping
    
    // Lighting basic pour commencer
    vec3 light_dir = normalize(vec3(0.5, 1.0, 0.3));
    float ndotl = max(dot(normal, light_dir), 0.0);
    
    color = vec4(albedo * (0.2 + 0.8 * ndotl), 1.0);
}
```

### 1.4 Synchronisation et performance

**Objects de synchronisation nécessaires** :
- **Semaphores** : Image disponible, rendu terminé
- **Fences** : Synchronisation CPU-GPU par frame
- **Barriers** : Transitions de layout d'images

**Pattern recommandé** :
```c
// Structure par frame
typedef struct VulkanFrame {
    VkCommandBuffer cmd_buffer;
    VkSemaphore image_available;
    VkSemaphore render_finished;  
    VkFence in_flight;
    VkDescriptorSet global_desc_set;
} VulkanFrame;
```

## 2. Fallback automatique vers les sprites

### 2.1 Système de détection d'échec 3D

**Conditions de fallback** :
1. Échec de création du pipeline Vulkan
2. Modèle glTF corrompu ou non supporté
3. Mémoire GPU insuffisante
4. Driver Vulkan indisponible

**Implémentation** :
```c
typedef enum RenderMode {
    RENDER_MODE_VULKAN_3D,
    RENDER_MODE_VULKAN_SPRITE,  
    RENDER_MODE_SOFTWARE_FALLBACK
} RenderMode;

typedef struct FallbackManager {
    RenderMode current_mode;
    RenderMode preferred_mode;
    bool vulkan_available;
    bool pipeline_3d_ready;
    char error_log[512];
} FallbackManager;
```
