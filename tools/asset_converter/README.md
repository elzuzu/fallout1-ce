# Sprite to Placeholder 3D Model Converter (glTF)

This tool converts a 2D sprite image (e.g., a PNG extracted from an FRM file) into a simple placeholder 3D model in glTF 2.0 format (.glb). The generated model consists of a single textured quad (a flat plane) with the sprite image applied as its texture.

This is useful for creating basic 3D representations of 2D assets for use in a 3D game engine, especially when full 3D models are not yet available or would be too complex to generate automatically.

## Features

*   Takes a PNG image as input.
*   Generates a glTF 2.0 binary file (.glb).
*   The glTF model contains a single quad mesh.
*   The quad is textured with the input PNG image.
*   UV coordinates map the entire image to the quad.
*   Vertex positions are based on image dimensions (scaled by a configurable factor).
*   The origin of the quad can be adjusted (e.g., bottom-center, center).

## Dependencies

*   Python 3.x
*   Pillow (PIL fork): `pip install Pillow`
*   pygltflib: `pip install pygltflib` (for creating the glTF structure)
*   NumPy: `pip install numpy` (often a dependency for geometry or binary packing, pygltflib might use it or it's good for buffer creation)

## Usage

```bash
python sprite_to_gltf.py <input_png_file> -o <output_glb_file> [--scale <factor>] [--origin <type>]
```

**Arguments:**

*   `input_png_file`: Path to the input PNG sprite image.
*   `-o`, `--output_file`: Path where the output .glb file will be saved.
*   `--scale`: (Optional) A floating-point scale factor to apply to the quad's dimensions (default: 0.01, assuming pixel dimensions are large). This helps convert pixel units to world units.
*   `--origin`: (Optional) Sets the origin of the quad. Supported values:
    *   `center` (default): Origin is at the center of the quad.
    *   `bottomcenter`: Origin is at the middle of the bottom edge.
    *   `bottomleft`: Origin is at the bottom-left corner.
    *   Custom pivot points are not yet supported via command line but could be added.

**Example:**

```bash
python sprite_to_gltf.py character_frame.png -o character_model.glb --scale 0.015 --origin bottomcenter
```

This will:
1.  Read `character_frame.png`.
2.  Create `character_model.glb`.
3.  The quad in the .glb will have dimensions based on `character_frame.png`, scaled by 0.015.
4.  The quad's origin (pivot point) will be at the center of its bottom edge.

## Output glTF Structure

The generated .glb file will contain:
*   A single scene with a single node.
*   The node references a mesh.
*   The mesh has one primitive (the quad) with vertex attributes for POSITION, NORMAL, and TEXCOORD_0.
*   A simple material that references the base color texture.
*   Texture, image, and sampler definitions for the input PNG.
*   Buffers, bufferViews, and accessors for the geometry data.
The image data is embedded within the .glb file.
