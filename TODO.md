# TODO

## État actuel

- [x] Rendu SDL classique fonctionnel.
- [x] Rendu Vulkan expérimental disponible via `RENDER_BACKEND=VULKAN`.
  - Upscaling via blit et surfaces SDL minimales.
  - Sélection du backend au lancement (modification `winmain`).
- [ ] Pas encore d'intégration 3D ou de pipeline hybride.

## À faire

### Infrastructure Vulkan/3D
- [ ] Compléter le moteur Vulkan (pipeline 3D complet, gestion commandes, shaders).
- [ ] Gestionnaire de ressources hybride (sprites + modèles 3D).
- [ ] Allocateur mémoire Vulkan avec pools pour vertex/index/uniformes.
- [ ] Fallback automatique vers les sprites en cas d'échec 3D.
- [ ] Support de modèles glTF 2.0 (chargement, materials, animations).

### Intégration dans la boucle de jeu
- [ ] Adaptation des entités (critter, objets) pour choisir entre sprite et modèle.
- [ ] Caméra isométrique 3D et gestion des rotations libres du joueur.
- [ ] Système d'animation interpolé compatible FRM.

### Pipeline d'assets
- [ ] Scripts d'extraction des FRM existants.
- [ ] Outils de conversion vers modèles 3D (Blender ou pipeline automatisé).
- [ ] Système de cache et de configuration par catégories (`f1_res.ini`).

### Phases de développement
- [ ] **Phase 1 :** mise en place du rendu 3D pour un seul asset test (joueur masculin).
- [ ] **Phase 2 :** généralisation à toutes les variantes du joueur et premières créatures.
- [ ] **Phase 3 :** extension contrôlée (quelques créatures et portes principales).
- [ ] **Phase 4 :** expansion (objets interactifs, optimisations LOD, instancing).
- [ ] **Phase 5 :** polish final et documentation.

