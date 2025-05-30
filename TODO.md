# TODO

## État actuel

- [x] Rendu SDL classique fonctionnel.
- [x] Rendu Vulkan expérimental disponible via `RENDER_BACKEND=VULKAN_BATCH`.
  - Upscaling via blit et surfaces SDL minimales.
  - Sélection du backend au lancement (modification `winmain`).
- [ ] Intégration 3D partielle : pipeline Vulkan 3D, caméra isométrique et chargeur glTF en développement.
- [x] Pipeline Vulkan 2D basique pour les sprites.
- [x] Gestionnaire de ressources avec cache (`ResourceManager`) et parser `AssetConfig`.
- [x] Allocateur mémoire Vulkan via VMA.
- [x] Script `frm_extract.py` pour extraire les FRM et option glTF.

## À faire

### Infrastructure Vulkan/3D
- [ ] Compléter le moteur Vulkan (pipeline 3D complet, gestion commandes, shaders).
- [x] Gestionnaire de ressources hybride (sprites + modèles 3D) initial implémenté.
- [x] Allocateur mémoire Vulkan avec pools pour vertex/index/uniformes.
- [ ] Fallback automatique vers les sprites en cas d'échec 3D.
- [ ] Support de modèles glTF 2.0 (chargement, materials, animations).

### Intégration dans la boucle de jeu
- [ ] Adaptation des entités (critter, objets) pour choisir entre sprite et modèle.
- [x] Caméra isométrique 3D et gestion des rotations libres du joueur.
- [ ] Système d'animation interpolé compatible FRM.

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
