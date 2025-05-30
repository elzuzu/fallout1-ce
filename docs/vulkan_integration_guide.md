Guide d'implémentation Vulkan pour Fallout1-CEVue d'ensemble du pipeline VulkanVoici l'architecture du pipeline graphique Vulkan d'après la documentation officielle :INPUT DATA → VERTEX SHADER → TESSELLATION → GEOMETRY → RASTERIZATION → FRAGMENT SHADER → COLOR BLEND → FRAMEBUFFER
     ↓                       ↓             ↓           ↓                 ↓               ↓             ↓
  Vertices               Transform       Subdivide   Add/Remove        Pixel Gen       Color Calc    Final Output
  Indices                Positions       Geometry    Primitives        Fragments       Texturing     Blending
  Attributes             Lighting                                                      Lighting
Légende des étapes :🟡 Programmables : Vertex Shader, Tessellation, Geometry, Fragment Shader🟢 Fixes : Input Assembly, Rasterization, Color Blend, Output Merger1. Compléter le moteur Vulkan (Pipeline 3D complet)1.1 Architecture actuelle et extensions nécessairesÉtat actuel : Votre projet a déjà un renderer Vulkan expérimental activé via RENDER_BACKEND=VULKAN.Extensions Vulkan à vérifier :// Dans votre VkInstance
VK_KHR_surface
VK_KHR_win32_surface  // Pour Windows
VK_EXT_debug_utils    // Pour le debug

// Dans votre VkDevice  
VK_KHR_swapchain
VK_EXT_graphics_pipeline_library  // Optionnel pour performance
1.2 Structure du pipeline 3D completComposants principaux à implémenter :A. Command Pool & Command Bufferstypedef struct VulkanCommandManager {
    VkCommandPool graphics_pool;
    VkCommandPool transfer_pool;
    VkCommandBuffer* primary_buffers;
    VkCommandBuffer* secondary_buffers;
    uint32_t frame_count;
} VulkanCommandManager;
Étapes d'implémentation :Créer des command pools séparés pour graphics et transfertAllouer des command buffers pour chaque frame en flight (double/triple buffering)Implémenter la réinitialisation des command buffers par frameB. Pipeline States pour différents usagesPipelines nécessaires :Sprite Pipeline (actuel) - pour compatibility fallbackModel Pipeline - pour les modèles 3D glTFUI Pipeline - pour l'interface FalloutPost-Process Pipeline - pour les effetsStructure de base :typedef struct VulkanPipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
    VkDescriptorSetLayout desc_layout;
    VkRenderPass render_pass;
    shader_type_t type;
} VulkanPipeline;
C. Gestion des ressources et descriptorsSystème de descriptor sets :Set 0 : Données globales (matrices view/projection, lighting)Set 1 : Données par objet (matrice model, material ID)Set 2 : Textures et samplersSet 3 : Données spécialisées (animation, morphing)1.3 Shaders essentiels à créerVertex Shader (vertex.vert)#version 450

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
Fragment Shader (fragment.frag)#version 450

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
1.4 Synchronisation et performanceObjects de synchronisation nécessaires :Semaphores : Image disponible, rendu terminéFences : Synchronisation CPU-GPU par frameBarriers : Transitions de layout d'imagesPattern recommandé :// Structure par frame
typedef struct VulkanFrame {
    VkCommandBuffer cmd_buffer;
    VkSemaphore image_available;
    VkSemaphore render_finished;  
    VkFence in_flight;
    VkDescriptorSet global_desc_set;
} VulkanFrame;
2. Fallback automatique vers les sprites2.1 Système de détection d'échec 3DConditions de fallback :Échec de création du pipeline VulkanModèle glTF corrompu ou non supportéMémoire GPU insuffisanteDriver Vulkan indisponibleImplémentation :typedef enum RenderMode {
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
3. Intégration de glTF et Fallout1-CEThe build integrates a lightweight copy of tinygltf located in external/tinygltf.No additional dependencies are required. Enable or disable glTF tests through thestandard f1_tests target.3.1 Points d'intégrationFichiers à modifier :src/game/main.c : Sélection du renderersrc/render/ : Nouveau module Vulkan 3Dsrc/game/critter.c : Rendu des personnages 3Dsrc/game/object.c : Rendu des objets de décor3.2 Configuration utilisateurNouvelles options dans f1_res.ini :[GRAPHICS]
RENDER_BACKEND=VULKAN
VULKAN_3D_MODELS=1
FALLBACK_TO_SPRITES=1
GLTF_MODEL_PATH=data/models/
ANIMATION_QUALITY=HIGH
PBR_SHADING=1
3.3 Performance et debuggingMétriques à exposer :FPS et frame timeMémoire GPU utiliséeNombre de draw callsTaille des descriptor setsTemps de chargement glTFDebug features :Wireframe modeNormal visualizationTexture atlas viewerAnimation timeline scrubber