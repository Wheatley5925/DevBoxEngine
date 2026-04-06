from __future__ import annotations

import struct
from pathlib import Path
from typing import Tuple

from PIL import Image
import json
from .pack_bitmap import load_grayscale, to_4bpp_packed

MAGIC_ATL = b"ATL4"

transparent_color = 0   #transparent color is hardcoded right now as black

def decode_json(src: Path) -> dict:
    with open(src, 'r') as f:
        data = json.load(f)
        frameCount = len(data['frames'])
        transparentColor = transparent_color
        reserved = 0
        
        frames = []
        for name, values in data['frames'].items():
            frame = [
                    name,
                    values['frame']['x'],
                    values['frame']['y'],
                    values['frame']['w'],
                    values['frame']['h'],
                    values['spriteSourceSize']['x'],    # originX
                    values['spriteSourceSize']['y']     # originY
                    ]
            frames.append(frame)
        return frameCount, transparentColor, reserved, frames


def write_atl(json_path: Path, out_path: Path, data4:bytes, width: int, height: int) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    count, transparent, reserved, frames = decode_json(json_path)
    header = struct.pack("<4sHHHBB", MAGIC_ATL, width, height, count, transparent, reserved)

    with out_path.open("wb") as f:
        f.write(header)

        for frame in frames:
            name_bytes = frame[0].encode("utf-8")[:15]
            name_bytes = name_bytes + b"\0" * (16 - len(name_bytes))
            frame_record = struct.pack("<16sHHHHhh", name_bytes, frame[1], frame[2], 
                                       frame[3], frame[4], frame[5], frame[6])    
            f.write(frame_record)

        f.write(data4)

def pack_atlas(src: Path, json_path: Path, dst: Path) -> None:
    im_l = load_grayscale(src)
    data4, w, h = to_4bpp_packed(im_l)
    write_atl(json_path, dst, data4, w, h)
