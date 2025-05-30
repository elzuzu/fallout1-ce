import argparse
import os
import struct
import json # pygltflib uses json for parts of its structure
import base64 # For embedding buffer data if not creating a .bin file separately
from PIL import Image
import pygltflib
import numpy as np

def image_to_base64_data_uri(image_path):
    """Converts an image file to a base64 data URI."""
    try:
        img = Image.open(image_path)
        mime_type = Image.MIME.get(img.format)
        if not mime_type:
            # Guess common types or default
            if image_path.lower().endswith(".png"):
                mime_type = "image/png"
            elif image_path.lower().endswith(".jpg") or image_path.lower().endswith(".jpeg"):
                mime_type = "image/jpeg"
            else: # Fallback, though glTF prefers png/jpeg
                mime_type = "application/octet-stream"

        with open(image_path, "rb") as f:
            encoded_string = base64.b64encode(f.read()).decode("utf-8")
        return f"data:{mime_type};base64,{encoded_string}"
    except Exception as e:
        print(f"Warning: Could not convert image {image_path} to data URI: {e}")
        return None


def main():
    parser = argparse.ArgumentParser(description="Convert a sprite PNG to a placeholder 3D model (textured quad) in GLB format.")
    parser.add_argument("input_png_file", help="Path to the input PNG sprite image.")
    parser.add_argument("-o", "--output_file", required=True, help="Path for the output .glb file.")
    parser.add_argument("--scale", type=float, default=0.01, help="Scale factor for the quad dimensions (pixels to world units). Default: 0.01")
    parser.add_argument("--origin", choices=['center', 'bottomcenter', 'bottomleft'], default='center', help="Sets the origin of the quad. Default: center")

    args = parser.parse_args()

    if not os.path.exists(args.input_png_file):
        print(f"Error: Input PNG file '{args.input_png_file}' not found.")
        return

    try:
        img = Image.open(args.input_png_file)
        img_width, img_height = img.size
        # Ensure image is RGBA for consistency, though glTF can handle RGB too
        if img.mode != 'RGBA':
            img = img.convert('RGBA')
            print(f"Converted image {args.input_png_file} to RGBA.")

    except Exception as e:
        print(f"Error opening or processing image '{args.input_png_file}': {e}")
        return

    # Quad dimensions based on image size and scale
    quad_width = img_width * args.scale
    quad_height = img_height * args.scale

    # Adjust vertex positions based on origin
    if args.origin == 'center':
        min_x, max_x = -quad_width / 2, quad_width / 2
        min_y, max_y = -quad_height / 2, quad_height / 2
    elif args.origin == 'bottomcenter':
        min_x, max_x = -quad_width / 2, quad_width / 2
        min_y, max_y = 0, quad_height
    elif args.origin == 'bottomleft':
        min_x, max_x = 0, quad_width
        min_y, max_y = 0, quad_height
    else: # Should not happen due to choices in argparse
        min_x, max_x = -quad_width / 2, quad_width / 2
        min_y, max_y = -quad_height / 2, quad_height / 2

    # Vertices: Position (X, Y, Z), Normal (X, Y, Z), UV (U, V)
    # Quad on XY plane, facing +Z. Normals point to +Z.
    # Z = 0 for all vertices.
    # Positions (counter-clockwise starting bottom-left for this example)
    # UVs (0,0 is bottom-left in glTF texture coordinates)
    vertex_data = np.array([
        # Position        Normal            UV
        min_x, min_y, 0.0,  0.0, 0.0, 1.0,  0.0, 0.0, # Bottom-left
        max_x, min_y, 0.0,  0.0, 0.0, 1.0,  1.0, 0.0, # Bottom-right
        max_x, max_y, 0.0,  0.0, 0.0, 1.0,  1.0, 1.0, # Top-right
        min_x, max_y, 0.0,  0.0, 0.0, 1.0,  0.0, 1.0, # Top-left
    ], dtype=np.float32)

    indices = np.array([
        0, 1, 2,  # First triangle
        0, 2, 3   # Second triangle
    ], dtype=np.uint16) # Use uint16 for small number of vertices

    vertex_buffer_bytes = vertex_data.tobytes()
    indices_buffer_bytes = indices.tobytes()

    # Combine into a single buffer for glTF
    # Add padding for alignment if necessary, though for this simple case it might not be critical
    # For GLB, data can be contiguous.
    # Pad vertex data to multiple of 4 bytes (already float32)
    # Pad index data to multiple of 2 bytes (already uint16)

    # For GLB, the binary chunk comes after the JSON.
    # We can create one large buffer and use bufferViews to slice it.
    # Or, pygltflib can pack separate byte strings into the GLB.

    # Buffer 0: Vertex data followed by Index data
    # For simplicity, let's make them separate buffers in terms of bytes,
    # then pygltflib will pack them into the GLB.

    buffer0_data = vertex_buffer_bytes
    buffer1_data = indices_buffer_bytes

    gltf = pygltflib.GLTF2()

    # Scene
    gltf.scene = 0
    gltf.scenes.append(pygltflib.Scene(nodes=[0])) # Node 0 is our quad

    # Node
    gltf.nodes.append(pygltflib.Node(mesh=0)) # Mesh 0

    # Mesh
    gltf.meshes.append(pygltflib.Mesh(
        primitives=[
            pygltflib.Primitive(
                attributes=pygltflib.Attributes(
                    POSITION=0, # Accessor 0 for positions
                    NORMAL=1,   # Accessor 1 for normals
                    TEXCOORD_0=2 # Accessor 2 for UVs
                ),
                indices=3 # Accessor 3 for indices
            )
        ]
    ))

    # Accessors
    # Accessor 0: Vertex Positions
    gltf.accessors.append(pygltflib.Accessor(
        bufferView=0, # BufferView 0
        componentType=pygltflib.FLOAT,
        count=4, # 4 vertices
        type=pygltflib.VEC3,
        max=[max_x, max_y, 0.0], # Bounding box for positions
        min=[min_x, min_y, 0.0]
    ))
    # Accessor 1: Vertex Normals
    gltf.accessors.append(pygltflib.Accessor(
        bufferView=1, # BufferView 1
        componentType=pygltflib.FLOAT,
        count=4,
        type=pygltflib.VEC3
    ))
    # Accessor 2: Vertex UVs
    gltf.accessors.append(pygltflib.Accessor(
        bufferView=2, # BufferView 2
        componentType=pygltflib.FLOAT,
        count=4,
        type=pygltflib.VEC2
    ))
    # Accessor 3: Indices
    gltf.accessors.append(pygltflib.Accessor(
        bufferView=3, # BufferView 3
        componentType=pygltflib.UNSIGNED_SHORT,
        count=len(indices), # 6 indices
        type=pygltflib.SCALAR
    ))

    # BufferViews
    # For interleaved vertex data (POS, NRM, UV in one bufferView)
    vertex_stride = 3*4 + 3*4 + 2*4 # (Pos_XYZ + Nrm_XYZ + UV_XY) * sizeof(float)
    gltf.bufferViews.append(pygltflib.BufferView( # BufferView 0 for Positions
        buffer=0, # Buffer 0
        byteOffset=0,
        byteLength=4 * 3 * 4, # 4 vertices * 3 floats * 4 bytes/float
        byteStride=vertex_stride,
        target=pygltflib.ARRAY_BUFFER
    ))
    gltf.bufferViews.append(pygltflib.BufferView( # BufferView 1 for Normals
        buffer=0,
        byteOffset=3 * 4, # Offset for Pos
        byteLength=4 * 3 * 4,
        byteStride=vertex_stride,
        target=pygltflib.ARRAY_BUFFER
    ))
    gltf.bufferViews.append(pygltflib.BufferView( # BufferView 2 for UVs
        buffer=0,
        byteOffset=(3+3) * 4, # Offset for Pos+Nrm
        byteLength=4 * 2 * 4,
        byteStride=vertex_stride,
        target=pygltflib.ARRAY_BUFFER
    ))
    gltf.bufferViews.append(pygltflib.BufferView( # BufferView 3 for Indices
        buffer=1, # Separate buffer for indices
        byteOffset=0,
        byteLength=len(indices) * 2, # num_indices * sizeof(uint16)
        target=pygltflib.ELEMENT_ARRAY_BUFFER
    ))

    # Buffers
    # pygltflib can auto-embed data for GLB or use filenames for .gltf
    # For GLB, we provide the bytes directly.
    gltf.buffers.append(pygltflib.Buffer(byteLength=len(vertex_buffer_bytes))) # Buffer 0 for vertex data
    gltf.buffers.append(pygltflib.Buffer(byteLength=len(indices_buffer_bytes))) # Buffer 1 for index data

    # Store actual buffer data to be packed into GLB's binary chunk
    # pygltflib handles this if data is added to gltf.binary_blob() or if buffer.uri is data URI
    # For GLB export, pygltflib expects the actual data to be added via gltf.set_binary_blob()
    # or by converting buffers to data URIs.
    # A simpler way for GLB with pygltflib is to let it manage the blob.
    # We'll provide the data when saving.

    # Image, Texture, Material
    # For GLB, image can be embedded via a bufferView, or a data URI.
    # pygltflib handles embedding if image.uri is a local path and saving as GLB.
    # Or, we can create a data URI for the image.

    # Option 1: Let pygltflib handle embedding the image from file path
    # gltf.images.append(pygltflib.Image(uri=os.path.abspath(args.input_png_file)))

    # Option 2: Embed image as data URI (more self-contained GLB if source image might move)
    # This makes the GLB larger but more portable.
    image_data_uri = image_to_base64_data_uri(args.input_png_file)
    if image_data_uri:
        gltf.images.append(pygltflib.Image(uri=image_data_uri))
    else: # Fallback if URI conversion failed, try direct path (might not embed in GLB as easily by all tools)
        print(f"Warning: Could not create data URI for image. Using file path '{args.input_png_file}' in glTF.")
        # pygltflib might require the image to be in the same dir or a subdir for relative paths
        # when creating a GLB, or an absolute path.
        # For robustness with GLB embedding when path isn't a data URI, it's often better to load image bytes
        # into a buffer, then use a bufferView for the image. Let's stick to data URI for simplicity of embedding.
        gltf.images.append(pygltflib.Image(uri=os.path.relpath(args.input_png_file, start=os.path.dirname(args.output_file))))


    gltf.textures.append(pygltflib.Texture(source=0, sampler=0)) # Image 0, Sampler 0
    gltf.materials.append(pygltflib.Material(
        pbrMetallicRoughness=pygltflib.PbrMetallicRoughness(
            baseColorTexture=pygltflib.TextureInfo(index=0) # Texture 0
        ),
        name="SpriteMaterial",
        doubleSided=True # Sprites are often double-sided
    ))
    gltf.samplers.append(pygltflib.Sampler(magFilter=pygltflib.NEAREST, minFilter=pygltflib.NEAREST)) # Pixel art often uses nearest

    # Save as GLB
    # pygltflib needs the binary data for buffers to be combined into a single blob for GLB.
    # The library can do this, or we can construct it.
    # The library will pack data from buffer.uri (if data URI) or from a passed blob.

    # Create the binary blob:
    # The order should match buffer indices: buffer[0] data, then buffer[1] data.
    # pygltflib will handle this if we assign the bytes to `gltf.buffers[i].uri` as data URI,
    // or by using `add_to_bin_chunk` methods if they exist, or by manually creating the blob.
    // The easiest is to let pygltflib assemble it from data URIs or by saving to .glb which triggers conversion.

    # For pygltflib, if buffer.uri is None, it expects the data in gltf.binary_blob() for GLB export.
    # Let's prepare data for set_binary_blob or direct saving.
    # The library will correctly create the BIN chunk from buffers that don't have URIs.

    # Clear URIs for buffers we want to be in the binary chunk
    gltf.buffers[0].uri = None
    gltf.buffers[1].uri = None

    # pygltflib > 2.1.6 can automatically create the blob from buffer objects with data
    # For older versions, manual blob construction might be needed.
    # Let's assume a version that handles it, or save as .gltf + .bin first.

    # To ensure it works with various pygltflib versions for GLB:
    # Create a combined binary blob for GLB.
    # Buffers must be ordered. BufferView offsets are relative to their buffer's start in the blob.
    # Since we defined BufferView 0,1,2 on Buffer 0, and BufferView 3 on Buffer 1:
    # Blob = vertex_buffer_bytes + indices_buffer_bytes
    # And update Buffer 1's definition if it's part of the same blob in glTF structure
    # For simplicity with pygltflib, if buffers[i].uri is None, their data (if provided separately)
    # should be concatenated in order of buffer index to form the binary chunk.

    # Let's try a robust way: convert buffer data to data URIs.
    # This way, pygltflib will decode them and pack into the GLB binary chunk.
    vertex_data_uri = "data:application/octet-stream;base64," + base64.b64encode(vertex_buffer_bytes).decode('utf-8')
    indices_data_uri = "data:application/octet-stream;base64," + base64.b64encode(indices_buffer_bytes).decode('utf-8')

    gltf.buffers[0].uri = vertex_data_uri
    gltf.buffers[0].byteLength = len(vertex_buffer_bytes)
    gltf.buffers[1].uri = indices_data_uri
    gltf.buffers[1].byteLength = len(indices_buffer_bytes)


    gltf.save_binary(args.output_file) # Saves as GLB
    print(f"Successfully converted '{args.input_png_file}' to '{args.output_file}'")

if __name__ == "__main__":
    main()
