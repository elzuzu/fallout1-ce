#!/usr/bin/env python3
"""Simple FRM to PNG extractor.

This utility reads a Fallout FRM file, decodes the frames and writes
out PNG files using Pillow. A manifest json is generated describing the
sprite sheet. Optional --gltf flag will invoke Blender in command line
mode to convert the generated sheet into a glTF mesh animation.
This is only a lightweight implementation and supports the common FRM
format used by the game.
"""

import argparse
import json
import os
import struct
import subprocess
from pathlib import Path

from PIL import Image


def read_header(f):
    header = f.read(12)
    if len(header) != 12:
        raise ValueError("Invalid FRM file")
    fps, action_frame, dir_count, frame_count = struct.unpack('<IHHH', header)
    return fps, action_frame, dir_count, frame_count


def decode_frame(f, width, height):
    data = bytearray()
    while len(data) < width * height:
        byte = f.read(1)
        if not byte:
            break
        val = byte[0]
        if val == 0x80:  # run marker
            count = f.read(1)[0]
            value = f.read(1)[0]
            data.extend([value] * count)
        else:
            data.append(val)
    return bytes(data[: width * height])


def main():
    parser = argparse.ArgumentParser(description="Extract FRM frames")
    parser.add_argument("input", help="Path to FRM file")
    parser.add_argument("--out", required=True, help="Output directory")
    parser.add_argument("--gltf", action="store_true", help="Generate glTF via Blender")
    args = parser.parse_args()

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    with open(args.input, "rb") as f:
        fps, action_frame, dirs, frames = read_header(f)
        width, height = struct.unpack('<HH', f.read(4))
        frame_offsets = [struct.unpack('<I', f.read(4))[0] for _ in range(dirs * frames)]
        images = []
        for i, off in enumerate(frame_offsets):
            f.seek(off)
            pixels = decode_frame(f, width, height)
            img = Image.frombytes('P', (width, height), pixels)
            img_path = out_dir / f"frame_{i}.png"
            img.save(img_path)
            images.append(str(img_path))

    manifest = {
        "name": Path(args.input).stem,
        "frames": frames,
        "dir": "N",
        "png": images[0] if images else "",
        "atlas": "sprites.atlas",
    }
    manifest_path = out_dir / f"{Path(args.input).stem}.json"
    with open(manifest_path, "w") as mf:
        json.dump(manifest, mf, indent=2)

    if args.gltf and images:
        cmd = [
            "blender",
            "-b",
            "--python-expr",
            (
                "import bpy,sys;img=bpy.data.images.load(sys.argv[-2]);"
                "bpy.ops.mesh.primitive_plane_add();"
                "mat=bpy.data.materials.new('mat');"
                "tex=bpy.data.textures.new('tex', 'IMAGE');tex.image=img;"
                "mat.texture_slots.add().texture=tex;"
                "bpy.context.object.data.materials.append(mat);"
                "bpy.ops.export_scene.gltf(filepath=sys.argv[-1])"
            ),
            images[0],
            str(out_dir / f"{Path(args.input).stem}.gltf"),
        ]
        subprocess.run(cmd, check=False)


if __name__ == "__main__":
    main()
