from __future__ import annotations

import struct
from pathlib import Path
from typing import Tuple

from PIL import Image


MAGIC = b"SPR4"


def quantize_8_to_4(gray8: int) -> int:
    return (gray8 * 15 + 127) // 255


def load_grayscale(img_path: Path) -> Image.Image:
    im = Image.open(img_path)
    return im.convert("L")


def to_4bpp_packed(im_l: Image.Image) -> Tuple[bytes, int, int]:
    w, h = im_l.size
    pix = im_l.load()
    out = bytearray()

    for y in range(h):
        x = 0
        while x < w:
            g0 = quantize_8_to_4(pix[x, y])

            if x + 1 < w:
                g1 = quantize_8_to_4(pix[x + 1, y])
            else:
                g1 = 0

            out.append(((g1 & 0x0F) << 4) | (g0 & 0x0F))
            x += 2

    return bytes(out), w, h


def write_spr(out_path: Path, data4: bytes, width: int, height: int) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    header = struct.pack("<4sHH", MAGIC, width, height)

    with out_path.open("wb") as f:
        f.write(header)
        f.write(data4)


def pack_bitmap(src: Path, dst: Path) -> None:
    im_l = load_grayscale(src)
    data4, w, h = to_4bpp_packed(im_l)
    write_spr(dst, data4, w, h)
