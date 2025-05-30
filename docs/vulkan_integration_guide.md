# Guide d'intégration Vulkan pour Fallout1-CE

## 1. Analyse de l'architecture actuelle
- Utilisation de SDL2 pour le rendu.
- Architecture orientée sprites.
- Fichiers de configuration `fallout.cfg` et `f1_res.ini`.

## 2. Architecture Vulkan recommandée
```
src/
├── graphics/
│   ├── renderer_interface.h
│   ├── sdl_renderer.cpp
│   ├── vulkan/
│   │   ├── vulkan_renderer.cpp
│   │   ├── vulkan_instance.cpp
│   │   ├── vulkan_device.cpp
│   │   ├── vulkan_swapchain.cpp
│   │   ├── vulkan_pipeline.cpp
│   │   └── memory_allocator.cpp
│   └── renderer_factory.cpp
```
Cette structure isole le code spécifique à Vulkan.

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

## 10. Configuration `f1_res.ini`
```ini
[MAIN]
SCR_WIDTH=1280
SCR_HEIGHT=720
WINDOWED=1

[GRAPHICS]
RENDERER=VULKAN  # VULKAN, SDL, AUTO
VULKAN_DEBUG=0   # Validation layers
VULKAN_VSYNC=1   # V-sync
VULKAN_GPU_INDEX=0
```

### Options for glTF loading

The build integrates a lightweight copy of `tinygltf` located in `external/tinygltf`.
No additional dependencies are required. Enable or disable glTF tests through the
standard `f1_tests` target.
