# Outils de débogage Vulkan

Cette page résume les principaux outils utiles pour valider et analyser le rendu Vulkan de *Fallout1-CE* lorsqu'il est exécuté via SDL.

| Outil | Fonctionnalités clés | Cas d'usage pour Fallout1‑CE (via SDL‑Vulkan) |
|-------|---------------------|-----------------------------------------------|
| **Couches de validation Khronos** | Détection d'erreurs d'API, non‑conformités, problèmes de performance et divers avertissements. | Activées durant le développement pour vérifier les appels Vulkan issus du backend SDL. Permet de repérer les mauvaises utilisations de l'API et d'obtenir des messages détaillés en console. |
| **RenderDoc** | Capture de trame, inspection des appels Vulkan, visualisation des ressources GPU (textures, buffers), état complet du pipeline et débogage de géométrie. | Utilisé pour analyser une trame rendue par SDL‑Vulkan, identifier les goulots d'étranglement, étudier les artefacts visuels et vérifier l'état des ressources. Un raccourci clavier (F12) ouvre l'interface RenderDoc via le `DebugHud`. |

