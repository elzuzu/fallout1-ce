# Fallout Community Edition

Fallout Community Edition est une réimplémentation complète de Fallout offrant le même gameplay d'origine, des corrections de bugs du moteur et quelques améliorations de confort. Cette branche cible uniquement **Windows 64 bits**.

Il existe également [Fallout 2 Community Edition](https://github.com/alexbatalov/fallout2-ce).

## État du projet
- Fonctionne sur les versions modernes de Windows 64 bits.
- Rendu Vulkan expérimental disponible via `RENDER_BACKEND=VULKAN_BATCH` ou la variable d'environnement `FALLOUT_RENDER_BACKEND`.
- La disposition du clavier est détectée automatiquement sous Windows.

## Installation
Vous devez posséder le jeu pour y jouer. Achetez une copie sur [GOG](https://www.gog.com/game/fallout) ou [Steam](https://store.steampowered.com/app/38400). Téléchargez la [dernière version](https://github.com/alexbatalov/fallout1-ce/releases) ou compilez le projet depuis les sources. Vous pouvez également consulter la [version debug](https://github.com/alexbatalov/fallout1-ce/actions) destinée aux testeurs.

### Windows
Téléchargez `fallout-ce.exe` et copiez-le dans votre dossier `Fallout`. Il remplace `falloutw.exe`.

## Configuration
Le lanceur embarque désormais une configuration par défaut. Pour charger
`fallout.cfg` ou `f1_res.ini`, définissez la variable d'environnement
`F1CE_USE_CONFIG_FILES=1`.

Il est aussi possible d'ajuster certaines options via variables
d'environnement :

- `F1CE_WIDTH` et `F1CE_HEIGHT` — résolution souhaitée (min. 640×480).
- `F1CE_WINDOWED` — mettre `1` pour jouer en fenêtre.
- `F1CE_RENDER_BACKEND` — `SDL` ou `VULKAN_BATCH`.

Lorsque `F1CE_USE_CONFIG_FILES` est actif, le fichier principal reste
`fallout.cfg`. Selon votre distribution du jeu, les fichiers `master.dat`,
`critter.dat` et le dossier `data` peuvent être en minuscules ou en
majuscules. Vous pouvez soit mettre à jour les paramètres `master_dat`,
`critter_dat`, `master_patches` et `critter_patches` pour correspondre à vos
fichiers, soit renommer les fichiers pour qu'ils correspondent aux entrées du
fichier.

Le dossier `sound` (qui contient `music`) peut se trouver soit dans `data`, soit
à la racine du jeu. Ajustez `music_path1` en conséquence, par exemple
`data/sound/music/` ou `sound/music/`. Les fichiers de musique (`ACM`) doivent
être en majuscules, quel que soit l'emplacement du dossier `sound`.

`f1_res.ini` permet de contrôler la taille de la fenêtre et le mode plein écran
avec le format suivant :

```ini
[MAIN]
SCR_WIDTH=1280
SCR_HEIGHT=720
WINDOWED=1
RENDER_BACKEND=SDL
```

Utilisez `RENDER_BACKEND=VULKAN_BATCH` pour activer le rendu Vulkan expérimental. Ce réglage peut également être remplacé par la variable d'environnement `FALLOUT_RENDER_BACKEND`.

Recommandations :
- **Ordinateurs de bureau** : utilisez la taille que vous souhaitez.

Ces variables permettent de personnaliser sans modifier les fichiers de configuration.

## Contribuer
Voici quelques objectifs actuels. Ouvrez une issue si vous avez des suggestions ou des demandes de fonctionnalités.
- **Mettre à jour en v1.2**. Le projet est basé sur la Reference Edition qui implémente la v1.1 sortie en novembre 1997. Une version 1.2 parue en mars 1998 apporte au moins un support multilingue important.
- **Rétroporter certaines fonctionnalités de Fallout 2**. Fallout 2 (avec quelques ajouts de Sfall) a introduit de nombreuses améliorations au moteur original. Beaucoup méritent d'être intégrées à Fallout 1. Gardez à l'esprit qu'il s'agit d'un jeu différent avec un équilibre légèrement modifié.

## Licence
Le code source de ce dépôt est disponible sous la [Sustainable Use License](LICENSE.md).
