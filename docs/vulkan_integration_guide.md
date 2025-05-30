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
