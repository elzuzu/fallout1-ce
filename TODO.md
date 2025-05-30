# TODO

## État actuel

- [x] Rendu SDL classique fonctionnel.
- [x] Rendu Vulkan expérimental disponible via `RENDER_BACKEND=VULKAN_BATCH`.
  - Upscaling via blit et surfaces SDL minimales.
  - Sélection du backend au lancement (modification `winmain`).
- [ ] Intégration 3D partielle :
  - [x] Prototype du pipeline Vulkan 3D (`GraphicsPipeline3D`).
  - [x] Mise en place d'une caméra isométrique (`IsoCamera`/`IsometricCamera`).
  - [x] Chargeur glTF basique opérationnel (`GltfLoader`).
  - [x] Support des modèles glTF dans `ResourceManager`.
  - [x] Affichage d'un modèle glTF simple via le pipeline 3D.
  - [x] Liaison complète de la caméra au moteur de rendu.
  - [x] Gestion initiale des matériaux et textures.
- [x] Pipeline Vulkan 2D basique pour les sprites.
- [x] Gestionnaire de ressources avec cache (`ResourceManager`) et parser `AssetConfig`.
- [x] Allocateur mémoire Vulkan via VMA.
- [x] Gestion mémoire avancée via `FalloutMemoryManager`.
- [x] Pools de buffers dédiés via `VulkanResourceAllocator`.
- [x] Script `frm_extract.py` pour extraire les FRM et option glTF.
- [x] Documentation initiale du pipeline Vulkan (voir `docs/vulkan_implementation_guide.md`).

## À faire

### Infrastructure Vulkan/3D
 - [x] Compléter le moteur Vulkan (pipeline 3D complet, gestion commandes, shaders).
 - [x] Vérifier les extensions Vulkan nécessaires (VK_KHR_surface, VK_KHR_win32_surface, VK_EXT_debug_utils, VK_KHR_swapchain, VK_EXT_graphics_pipeline_library).
- [ ] Implémenter `VulkanCommandManager` (pools de commandes et réinitialisation par frame).
 - [x] Créer les pipelines `Sprite`, `Model`, `UI` et `Post-Process`.
 - [x] Organiser les descriptor sets (0 global, 1 objet, 2 textures, 3 spécial).
- [x] Ajouter les shaders de base `vertex.vert` et `fragment.frag`.
- [x] Pipeline de post-traitement AAA avec presets spécifiques Fallout.
- [ ] Utiliser `VulkanFrame` pour la synchronisation par frame.
 - [x] Mettre en place le `FallbackManager` pour détecter les échecs 3D.
- [x] Recréation du swapchain tolérante aux pertes de périphérique.
- [x] LUT palette GPU et shader dédié.
- [x] Pipeline compute de conversion YUV420 → RGBA.
- [x] Cache de pipeline persistant.
- [x] Buffer uniforme persistant avec adresse GPU.
- [x] Gestionnaire de ressources hybride (sprites + modèles 3D) initial implémenté.
- [x] Renderer hybride 2D/3D opérationnel.
- [x] Allocateur mémoire Vulkan avec pools pour vertex/index/uniformes.
- [x] HUD de debug ImGui avec capture RenderDoc (F12).
- [x] Descriptor indexing pour atlases multiples.
- [x] Command buffers secondaires enregistrés en multithread.
- [x] Rendu dynamique via VK_KHR_dynamic_rendering.
- [x] Détection MoltenVK et chemins fallback.
- [x] Hot-reload des shaders GLSL/HLSL.
- [ ] Génération automatique des mipmaps pour les textures.
- [x] Fallback automatique vers les sprites en cas d'échec 3D.
- [x] Prototype `IsoCamera` et culling CPU pour tests.
 - [x] Support de modèles glTF 2.0 (chargement, materials, animations).
- [ ] Implémenter une transition intelligente Sprite/3D avec cache des sprites.
- [ ] Mettre en place une interface de rendu unifiée (`RenderObject`).

### Intégration dans la boucle de jeu
- [ ] Adaptation des entités (critter, objets) pour choisir entre sprite et modèle.
- [x] Caméra isométrique 3D et gestion des rotations libres du joueur.
 - [x] Système d'animation interpolé compatible FRM.

### Pipeline d'assets
- [x] Scripts d'extraction des FRM existants.
- [x] Outils de conversion vers modèles 3D (Blender ou pipeline automatisé).
- [x] Système de cache et de configuration par catégories (`f1_res.ini`).

### Phases de développement
- [x] **Phase 1 :** mise en place du rendu 3D pour un seul asset test (joueur masculin).
- [ ] **Phase 2 :** généralisation à toutes les variantes du joueur et premières créatures.
- [ ] **Phase 3 :** extension contrôlée (quelques créatures et portes principales).
- [ ] **Phase 4 :** expansion (objets interactifs, optimisations LOD, instancing).
- [ ] **Phase 5 :** polish final et documentation.
