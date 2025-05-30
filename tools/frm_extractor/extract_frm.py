import struct
import argparse
import os
import json
from PIL import Image
from palette import load_palette

def read_frm_header(f):
    """Reads the FRM header."""
    header_data = {}
    header_data['version'] = struct.unpack('<I', f.read(4))[0]
    header_data['fps'] = struct.unpack('<H', f.read(2))[0]
    header_data['action_frame'] = struct.unpack('<H', f.read(2))[0]
    header_data['frames_per_direction'] = struct.unpack('<H', f.read(2))[0]

    shift_x = list(struct.unpack('<6h', f.read(12))) # short (2 bytes) * 6
    shift_y = list(struct.unpack('<6h', f.read(12))) # short (2 bytes) * 6
    frame_data_offsets = list(struct.unpack('<6I', f.read(24))) # unsigned int (4 bytes) * 6
    header_data['frame_area_size'] = struct.unpack('<I', f.read(4))[0]

    header_data['directions'] = []
    for i in range(6): # Max 6 directions
        header_data['directions'].append({
            'shift_x': shift_x[i],
            'shift_y': shift_y[i],
            'frame_data_offset': frame_data_offsets[i]
        })
    return header_data

def read_frame_header(f):
    """Reads the header for a single frame."""
    frame_h_data = {}
    frame_h_data['width'] = struct.unpack('<H', f.read(2))[0]
    frame_h_data['height'] = struct.unpack('<H', f.read(2))[0]
    frame_h_data['pixel_data_size'] = struct.unpack('<I', f.read(4))[0]
    frame_h_data['offset_x'] = struct.unpack('<h', f.read(2))[0]
    frame_h_data['offset_y'] = struct.unpack('<h', f.read(2))[0]
    return frame_h_data

def main():
    parser = argparse.ArgumentParser(description="Extracts frames from Fallout FRM files.")
    parser.add_argument("input_frm_file", help="Path to the FRM file.")
    parser.add_argument("-o", "--output_dir", required=True, help="Directory to save extracted frames.")
    parser.add_argument("-p", "--palette_file", help="Path to the palette file (e.g., COLOR.PAL).")
    parser.add_argument("--json", action="store_true", help="Output metadata as JSON.")

    args = parser.parse_args()

    if not os.path.exists(args.input_frm_file):
        print(f"Error: Input FRM file '{args.input_frm_file}' not found.")
        return

    os.makedirs(args.output_dir, exist_ok=True)

    palette_rgba = load_palette(args.palette_file)

    all_frames_metadata = []

    try:
        with open(args.input_frm_file, 'rb') as f:
            frm_header = read_frm_header(f)
            print(f"FRM Version: {frm_header['version']}")
            print(f"FPS: {frm_header['fps']}")
            print(f"Frames per Direction: {frm_header['frames_per_direction']}")

            # Determine if this FRM has actual multiple directions or if it's a single sequence
            # Some FRMs might use only direction 0 but store all frames under it.
            # If frame_data_offsets for dir > 0 are 0, it means those directions are not present or empty.
            active_directions = 0
            for i in range(6):
                # A direction is considered active if its offset is non-zero,
                # or if it's direction 0 (always assumed to be potentially active).
                # Also, if frames_per_direction is 0, it might mean all frames are in the first direction.
                if frm_header['frames_per_direction'] == 0: # All frames in first direction
                    if i == 0:
                         active_directions = 1
                    break
                if frm_header['directions'][i]['frame_data_offset'] != 0 or i == 0:
                    # Check if this offset is actually used or if next dir starts at same place
                    if i < 5 and frm_header['directions'][i+1]['frame_data_offset'] != 0 and \
                       frm_header['directions'][i+1]['frame_data_offset'] == frm_header['directions'][i]['frame_data_offset']:
                        if frm_header['frames_per_direction'] > 0: # Only break if this dir was supposed to have frames
                           pass # This direction is empty as next one starts at same place.
                        else: # If FPD is 0, this means only one direction block.
                            active_directions = i + 1 # tentative
                    else:
                        active_directions = i + 1
                else: # First zero offset for a direction > 0 means no more directions
                    if i > 0 : # Only break if not first direction
                        break

            if frm_header['frames_per_direction'] == 0: # Special case: all frames in one sequence
                num_frames_total_in_file = 0
                # Heuristic: count frames until data runs out or width/height is 0
                # This is tricky without knowing the total number of frames.
                # The 'frame_area_size' might be for *all* packed pixel data.
                # Let's assume for now FPD=0 means we read until frame_header width/height is 0
                # This part is often specific to how tools pack FRM files.
                # A common interpretation is that FPD=0 means the number of frames is unknown
                # and you read until you can't read a valid frame header.
                # For robust parsing, one might need to know how many frames are in this single sequence.
                # The provided FRM structure doesn't explicitly state total frames if FPD = 0.
                # Let's assume for this script: if FPD is 0, it implies a single direction,
                # and we try to read frames sequentially. The first frame header is at FrameDataOffset[0].
                # This is a simplification. A more robust way would be to sum all pixel_data_size
                # and see if it matches frame_area_size, but even that is not perfect.
                print("Warning: frames_per_direction is 0. Assuming single direction, will attempt to read frames sequentially.")
                # For this script, we will assume if FPD is 0, all frames are in direction 0.
                # The loop below will handle it by iterating frames_per_direction times.
                # If it's truly 0, the loop won't run. This needs a better heuristic.
                #
                # A common case for FPD=0 is that the FRM is not animated or has a single frame.
                # Let's try to read at least one frame if dir 0 offset is valid.
                if active_directions == 0 and frm_header['directions'][0]['frame_data_offset'] != 0:
                    active_directions = 1 # Force at least one direction to be processed if offset is there

                # If FPD is 0, we assume it's a single frame for each "active_direction" found by offsets.
                # This is a common pattern for static images stored in FRM format.
                # If it's an animation with FPD=0, this script will likely only get first frame per dir.
                # True FPD=0 handling is complex and depends on specific FRM producer.
                # For now, if FPD is 0, we'll try to read 1 frame per active direction.
                # This is a common scenario for static FRMs. If it's an animation with FPD=0, this is insufficient.
                _frames_to_read_this_dir = 1 if frm_header['frames_per_direction'] == 0 else frm_header['frames_per_direction']

            else: # FPD > 0
                 _frames_to_read_this_dir = frm_header['frames_per_direction']


            for dir_idx in range(active_directions):
                print(f"\nProcessing Direction {dir_idx}")
                print(f"  Shift X: {frm_header['directions'][dir_idx]['shift_x']}, Y: {frm_header['directions'][dir_idx]['shift_y']}")
                print(f"  Frame Data Offset: {frm_header['directions'][dir_idx]['frame_data_offset']}")

                f.seek(frm_header['directions'][dir_idx]['frame_data_offset'])

                # Determine actual frames to read for this direction
                # If FPD is 0, it might be a single frame FRM (e.g. inventory images)
                # or it could be an animation where FPD is implicitly defined by reading until EOF for that direction's block.
                # This script simplifies: if FPD is 0, assumes 1 frame. If FPD > 0, uses that count.
                # A more robust parser would need to handle animations with FPD=0 by other means (e.g. total size).

                frames_in_this_direction = _frames_to_read_this_dir
                if frm_header['frames_per_direction'] == 0 and active_directions > 1 :
                    # If FPD is 0 but we are processing multiple "directions" (e.g. FR0, FR1 files loaded as one logical FRM)
                    # this implies each direction file itself contains one frame sequence.
                    # This script is designed for one FRM file, so this case is less relevant here.
                    # If it's a multi-frame FRM with FPD=0, this needs a better heuristic.
                     pass


                for frame_idx in range(frames_in_this_direction):
                    # Store current file position, as frame_header reading consumes bytes
                    current_frame_data_start_offset = f.tell()
                    frame_h = read_frame_header(f)

                    if frame_h['width'] == 0 or frame_h['height'] == 0:
                        print(f"  Frame {frame_idx}: Width or Height is 0, assuming end of frames for this direction.")
                        if frm_header['frames_per_direction'] == 0 : break # Critical for FPD=0 case
                        else: continue # Skip this frame if FPD > 0

                    print(f"  Frame {frame_idx}: W={frame_h['width']}, H={frame_h['height']}, Size={frame_h['pixel_data_size']}")
                    print(f"    Offset X={frame_h['offset_x']}, Y={frame_h['offset_y']}")

                    frame_metadata = {
                        'direction': dir_idx,
                        'frame': frame_idx,
                        'width': frame_h['width'],
                        'height': frame_h['height'],
                        'offset_x': frame_h['offset_x'],
                        'offset_y': frame_h['offset_y'],
                        'shift_x': frm_header['directions'][dir_idx]['shift_x'],
                        'shift_y': frm_header['directions'][dir_idx]['shift_y'],
                        'source_file_offset': current_frame_data_start_offset
                    }

                    pixel_data_indexed = f.read(frame_h['pixel_data_size'])
                    if len(pixel_data_indexed) != frame_h['pixel_data_size']:
                        print(f"    Error: Could not read enough pixel data. Expected {frame_h['pixel_data_size']}, got {len(pixel_data_indexed)}")
                        # If FPD=0, this is a definite end.
                        if frm_header['frames_per_direction'] == 0 : break
                        else: continue # Skip this frame

                    # Create image from indexed data + palette
                    img = Image.new('RGBA', (frame_h['width'], frame_h['height']))
                    pixels = img.load()

                    for y in range(frame_h['height']):
                        for x in range(frame_h['width']):
                            idx_in_frame = y * frame_h['width'] + x
                            if idx_in_frame < len(pixel_data_indexed):
                                palette_idx = pixel_data_indexed[idx_in_frame]
                                if palette_idx < len(palette_rgba):
                                    pixels[x, y] = palette_rgba[palette_idx]
                                else: # Should not happen with valid FRM and palette
                                    pixels[x, y] = (255, 0, 255, 255) # Error color: Magenta
                            else: # Should not happen if pixel_data_size is correct
                                pixels[x,y] = (0,255,0,255) # Error color: Green

                    output_filename = f"dir{dir_idx}_frame{frame_idx}.png"
                    img.save(os.path.join(args.output_dir, output_filename))
                    frame_metadata['filename'] = output_filename
                    all_frames_metadata.append(frame_metadata)

                    if frm_header['frames_per_direction'] == 0 and frame_h['width'] > 0 and frame_h['height'] > 0:
                        # For FPD=0, if we successfully read one frame, we might need to check if there's another one
                        # This is where it gets tricky. A simple way is to peek at next frame's width/height.
                        next_pos = f.tell()
                        try:
                            next_w = struct.unpack('<H', f.read(2))[0]
                            next_h = struct.unpack('<H', f.read(2))[0]
                            f.seek(next_pos) # rewind
                            if next_w == 0 or next_h == 0:
                                print("    Next frame has zero width/height, ending FPD=0 sequence.")
                                break # End of frames for FPD=0
                        except struct.error: # Could not read, likely EOF
                            print("    Could not peek next frame header, ending FPD=0 sequence.")
                            break # End of frames for FPD=0


        if args.json:
            json_path = os.path.join(args.output_dir, "metadata.json")
            with open(json_path, 'w') as jf:
                json.dump({
                    "frm_header": {k: v for k, v in frm_header.items() if k != 'directions'}, # clean up directions for top level
                    "directions_summary": frm_header['directions'][:active_directions],
                    "extracted_frames": all_frames_metadata
                }, jf, indent=4)
            print(f"\nMetadata saved to {json_path}")

        print("\nExtraction complete.")

    except FileNotFoundError:
        print(f"Error: FRM file '{args.input_frm_file}' not found.")
    except Exception as e:
        print(f"An error occurred: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
