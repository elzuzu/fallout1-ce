import struct

DEFAULT_PALETTE_DATA = [ # Grayscale for fallback
    (i, i, i) for i in range(256)
]

def load_palette(palette_file_path=None):
    """
    Loads a palette from a .PAL file or returns a default.
    Fallout .PAL files are typically 256 RGB entries, 3 bytes each.
    The color values in Fallout PAL files are often in 0-63 range and
    need to be scaled to 0-255.
    """
    palette = []
    if palette_file_path:
        try:
            with open(palette_file_path, 'rb') as f:
                for _ in range(256):
                    r, g, b = struct.unpack('BBB', f.read(3))
                    # Scale Fallout's 0-63 range to 0-255
                    palette.append((r * 4, g * 4, b * 4))
            if not palette: # File might be too short or empty
                raise ValueError("Palette file contained no data.")
            # Ensure palette has 256 entries, pad if necessary (though PAL should be complete)
            while len(palette) < 256:
                palette.append((0,0,0)) # Pad with black if file is too short
            palette = palette[:256] # Truncate if too long

        except FileNotFoundError:
            print(f"Warning: Palette file '{palette_file_path}' not found. Using default grayscale palette.")
            palette = DEFAULT_PALETTE_DATA
        except Exception as e:
            print(f"Warning: Error loading palette '{palette_file_path}': {e}. Using default grayscale palette.")
            palette = DEFAULT_PALETTE_DATA
    else:
        # print("Info: No palette file provided. Using default grayscale palette.")
        # For a slightly better default, one could embed a known Fallout-like palette here.
        # For example, a simplified one or a common one.
        # Using grayscale as a simple, clearly indicated fallback.
        palette = DEFAULT_PALETTE_DATA

    # Ensure alpha channel for PNG output (index 0 is often transparent)
    # For many Fallout palettes, color index 0 is transparent black.
    final_palette_rgba = []
    for i, (r, g, b) in enumerate(palette):
        if i == 0: # Assuming index 0 is transparent
            final_palette_rgba.append((r, g, b, 0))
        else:
            final_palette_rgba.append((r, g, b, 255))

    return final_palette_rgba

if __name__ == '__main__':
    # Test loading a palette (e.g., if you have a COLOR.PAL)
    # pal = load_palette('path_to_your/COLOR.PAL')
    pal = load_palette() # Test default
    if pal:
        print(f"Loaded palette with {len(pal)} colors.")
        print("First 3 colors (RGBA):", pal[:3])
        print("Last 3 colors (RGBA):", pal[-3:])

        # Example of how the palette data is structured for Pillow's putpalette()
        # flat_palette = [val for color_tuple in pal for val in color_tuple[:3]] # For RGB P mode
        # However, we will convert to RGBA image directly, so this flattening isn't strictly needed for putpalette.
    else:
        print("Failed to load palette.")
