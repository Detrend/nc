#!/usr/bin/env python3
"""Downscale PNG textures in place."""

import fnmatch
import math
from pathlib import Path
import sys

from PIL import Image

if len(sys.argv) < 2:
    raise SystemExit(f"Usage: {Path(sys.argv[0]).name} SCALE_FACTOR [SKIP_PATTERN ...]")
try:
    scale = float(sys.argv[1])
except ValueError:
    raise SystemExit(f"Scale factor must be a positive number: {sys.argv[1]}")
if not math.isfinite(scale) or scale <= 0:
    raise SystemExit(f"Scale factor must be a positive number: {scale}")
skip_patterns = sys.argv[2:]

textures = Path(__file__).resolve().parent.parent / "content" / "textures"
files = sorted(path for path in textures.rglob("*") if path.is_file() and path.suffix.lower() == ".png")

count = 0
for path in files:
    relative_path = path.relative_to(textures).as_posix()
    if any(fnmatch.fnmatchcase(relative_path, pattern) for pattern in skip_patterns):
        continue
    with Image.open(path) as image:
        if image.mode == "P":
            image = image.convert("RGBA" if "transparency" in image.info else "RGB")
        if image.mode not in ("RGB", "RGBA", "L"):
            raise ValueError(f"Expected RGB, RGBA, L, or P image, got {image.mode}: {path}")
        width, height = image.size
        size = (max(1, round(width * scale)), max(1, round(height * scale)))
        resized = image.resize(size, Image.Resampling.LANCZOS)
        options = {
            key: image.info[key]
            for key in ("icc_profile", "exif", "dpi")
            if key in image.info
        }
        resized.save(path, **options)
    print(
        f"{relative_path}: {width}x{height} -> {size[0]}x{size[1]}"
    )
    count += 1
print(f"Downscaled {count} texture(s).")
