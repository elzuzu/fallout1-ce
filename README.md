# Fallout Community Edition

Fallout Community Edition is a fully working re-implementation of Fallout, with the same original gameplay, engine bugfixes, and some quality of life improvements. This fork targets **64-bit Windows** only.

There is also [Fallout 2 Community Edition](https://github.com/alexbatalov/fallout2-ce).

## Project Status

- Runs on modern **64-bit Windows** only.
- Experimental Vulkan renderer available via `RENDER_BACKEND=VULKAN_BATCH` or the
  `FALLOUT_RENDER_BACKEND` environment variable.
- Keyboard layout is detected automatically on Windows.

## Installation

You must own the game to play. Purchase your copy on [GOG](https://www.gog.com/game/fallout) or [Steam](https://store.steampowered.com/app/38400). Download latest [release](https://github.com/alexbatalov/fallout1-ce/releases) or build from source. You can also check latest [debug](https://github.com/alexbatalov/fallout1-ce/actions) build intended for testers.

### Windows

Download and copy `fallout-ce.exe` to your `Fallout` folder. It serves as a drop-in replacement for `falloutw.exe`.

## Configuration

The launcher contains default configuration so the game works out of the box. If
you want to load `fallout.cfg` or `f1_res.ini` anyway, set the environment
variable `F1CE_USE_CONFIG_FILES=1`.

You can also tweak a few options via environment variables:

- `F1CE_WIDTH` and `F1CE_HEIGHT` – desired resolution (minimum 640×480).
- `F1CE_WINDOWED` – set to `1` for windowed mode.
- `F1CE_RENDER_BACKEND` – `SDL` or `VULKAN_BATCH`.

When `F1CE_USE_CONFIG_FILES` is enabled the main configuration file is
`fallout.cfg`. Depending on your Fallout distribution main game assets
`master.dat`, `critter.dat`, and `data` folder might be either all lowercased or
all uppercased. You can either update `master_dat`, `critter_dat`,
`master_patches` and `critter_patches` settings to match your file names, or
rename files to match entries in your `fallout.cfg`.

The `sound` folder (with `music` folder inside) might be located either in `data`
folder, or be in the Fallout folder. Update `music_path1` accordingly, usually
it's `data/sound/music/` or `sound/music/`. Music files themselves (with `ACM`
extension) should be all uppercased, regardless of `sound` and `music` folders.

`f1_res.ini` controls window size and fullscreen mode and supports the following
format:

```ini
[MAIN]
SCR_WIDTH=1280
SCR_HEIGHT=720
WINDOWED=1
RENDER_BACKEND=SDL
```

Use `RENDER_BACKEND=VULKAN_BATCH` to enable the experimental Vulkan renderer. This
setting can also be overridden by the `FALLOUT_RENDER_BACKEND` environment
variable.

Recommendations:
- **Desktops**: Use any size you see fit.

These variables allow simple tweaks without editing configuration files.

## Contributing

Here is a couple of current goals. Open up an issue if you have suggestion or feature request.

- **Update to v1.2**. This project is based on Reference Edition which implements v1.1 released in November 1997. There is a newer v1.2 released in March 1998 which at least contains important multilingual support.

- **Backport some Fallout 2 features**. Fallout 2 (with some Sfall additions) added many great improvements and quality of life enhancements to the original Fallout engine. Many deserve to be backported to Fallout 1. Keep in mind this is a different game, with slightly different gameplay balance (which is a fragile thing on its own).

## License

The source code is this repository is available under the [Sustainable Use License](LICENSE.md).
