# Fallout Community Edition

Fallout Community Edition est une réimplémentation complète de Fallout offrant le même gameplay d'origine, des corrections de bugs du moteur et quelques améliorations de confort. Cette branche cible uniquement **Windows 64 bits**.

Il existe également [Fallout 2 Community Edition](https://github.com/alexbatalov/fallout2-ce).

## État du projet
- Fonctionne sur les versions modernes de Windows 64 bits.
- Rendu Vulkan expérimental disponible via `RENDER_BACKEND=VULKAN` ou la variable d'environnement `FALLOUT_RENDER_BACKEND`.
- La disposition du clavier est détectée automatiquement sous Windows.

## Installation
Vous devez posséder le jeu pour y jouer. Achetez une copie sur [GOG](https://www.gog.com/game/fallout) ou [Steam](https://store.steampowered.com/app/38400). Téléchargez la [dernière version](https://github.com/alexbatalov/fallout1-ce/releases) ou compilez le projet depuis les sources. Vous pouvez également consulter la [version debug](https://github.com/alexbatalov/fallout1-ce/actions) destinée aux testeurs.

### Windows
Téléchargez `fallout-ce.exe` et copiez-le dans votre dossier `Fallout`. Il remplace `falloutw.exe`.

## Configuration
Le fichier principal de configuration est `fallout.cfg`. Plusieurs réglages importants peuvent nécessiter une adaptation pour votre installation. Selon votre distribution du jeu, les fichiers `master.dat`, `critter.dat` et le dossier `data` peuvent être en minuscules ou en majuscules. Vous pouvez soit mettre à jour les paramètres `master_dat`, `critter_dat`, `master_patches` et `critter_patches` pour correspondre à vos fichiers, soit renommer les fichiers afin qu'ils correspondent aux entrées du fichier.

Le dossier `sound` (qui contient `music`) peut se trouver soit dans `data`, soit à la racine du jeu. Ajustez `music_path1` pour refléter votre organisation, par exemple `data/sound/music/` ou `sound/music/`. Les fichiers de musique (`ACM`) doivent être en majuscules, quel que soit l'emplacement du dossier `sound`.

Le second fichier de configuration est `f1_res.ini`. Utilisez-le pour changer la taille de la fenêtre de jeu et activer ou désactiver le plein écran.

```ini
[MAIN]
SCR_WIDTH=1280
SCR_HEIGHT=720
WINDOWED=1
RENDER_BACKEND=SDL
```

Utilisez `RENDER_BACKEND=VULKAN` pour activer le rendu Vulkan expérimental. Ce réglage peut également être remplacé par la variable d'environnement `FALLOUT_RENDER_BACKEND`.

Recommandations :
- **Ordinateurs de bureau** : utilisez la taille que vous souhaitez.
- **Tablettes** : définissez ces valeurs sur la résolution logique de votre appareil (par exemple, l'iPad Pro 11 fait 1668x2388 pixels mais sa résolution logique est de 834x1194 points).
- **Téléphones** : réglez la hauteur sur 480 et calculez la largeur selon le ratio de votre écran, par exemple un Samsung S21 (20:9) aura une largeur de `480 * 20 / 9 = 1067`.

Cette configuration disposera d'une interface intégrée ultérieurement. Pour l'instant elle doit être réalisée manuellement.

## Contribuer
Voici quelques objectifs actuels. Ouvrez une issue si vous avez des suggestions ou des demandes de fonctionnalités.
- **Mettre à jour en v1.2**. Le projet est basé sur la Reference Edition qui implémente la v1.1 sortie en novembre 1997. Une version 1.2 parue en mars 1998 apporte au moins un support multilingue important.
- **Rétroporter certaines fonctionnalités de Fallout 2**. Fallout 2 (avec quelques ajouts de Sfall) a introduit de nombreuses améliorations au moteur original. Beaucoup méritent d'être intégrées à Fallout 1. Gardez à l'esprit qu'il s'agit d'un jeu différent avec un équilibre légèrement modifié.

## Licence
Le code source de ce dépôt est disponible sous la [Sustainable Use License](LICENSE.md).
