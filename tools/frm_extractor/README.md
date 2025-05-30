# FRM Extractor for Fallout 1/2

This tool extracts frames from Fallout 1 and 2 FRM animation files and saves them as PNG images. It can also output metadata about the FRM file and its frames.

## FRM Format Background (Assumed)

The script is based on common understanding of the FRM format:

*   **Header:** Contains version, FPS, action frame, frames per direction, and offsets/shifts for each of the 6 possible animation directions.
*   **Frame Data per Direction:** For each direction, there's a block of data containing individual frames.
*   **Frame Header:** Each frame has its own header specifying width, height, pixel data size, and X/Y offsets for positioning.
*   **Pixel Data:** Frame pixel data is color-indexed, referring to a 256-color palette. Index 0 is typically transparent.

This extractor assumes a standard palette (like `COLOR.PAL` from Fallout) is used for converting indexed images to RGBA PNGs.

## Features

*   Parses FRM headers and individual frame headers.
*   Handles FRM files with single or multiple (up to 6) animation directions.
*   Extracts each frame's pixel data.
*   Applies a color palette (e.g., Fallout's `COLOR.PAL`) to convert indexed pixels to RGBA.
*   Saves extracted frames as individual PNG files.
*   Outputs metadata (frame dimensions, offsets) as a JSON file (optional).

## Dependencies

*   Python 3.x
*   Pillow (PIL fork): `pip install Pillow`

## Usage

```bash
python extract_frm.py <input_frm_file> -o <output_directory> [-p <palette_file>] [--json]
```

**Arguments:**

*   `input_frm_file`: Path to the FRM file to process.
*   `-o`, `--output_dir`: Directory where the extracted PNG frames and JSON metadata will be saved. The directory will be created if it doesn't exist.
*   `-p`, `--palette_file`: (Optional) Path to the palette file (e.g., `COLOR.PAL`). If not provided, a default grayscale palette will be used, or a common Fallout-like palette might be embedded. Palette entries are expected to be 3 bytes (RGB), with Fallout's 0-63 range automatically scaled to 0-255.
*   `--json`: (Optional) If specified, outputs a JSON file containing metadata for all frames (dimensions, offsets, direction).

**Example:**

```bash
python extract_frm.py HFJMPSLA.FRM -o extracted_frames -p ../gamedata/COLOR.PAL --json
```

This will:
1.  Read `HFJMPSLA.FRM`.
2.  Use `../gamedata/COLOR.PAL` for colors.
3.  Create an `extracted_frames` directory.
4.  Save PNG images for each frame (e.g., `extracted_frames/dir0_frame0.png`, `extracted_frames/dir0_frame1.png`, etc.).
5.  Save `extracted_frames/metadata.json` with details about each frame.

## Output File Naming

Extracted frames will be named according to the pattern: `dir<D>_frame<F>.png`
*   `<D>`: Direction index (0-5).
*   `<F>`: Frame index within that direction.

If the FRM only contains one direction (or is treated as such by having `FramesPerDirection` cover all frames), `<D>` might always be 0.
