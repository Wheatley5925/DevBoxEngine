from __future__ import annotations

import subprocess
from pathlib import Path


def pack_audio(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)

    cmd = [
        "ffmpeg",
        "-y",
        "-i", str(src),
        "-vn",
        "-ar", "44100",
        "-ac", "2",
        "-sample_fmt", "s16",
        "-f", "wav",
        str(dst),
    ]

    subprocess.run(cmd, check=True)
