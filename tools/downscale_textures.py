#!/usr/bin/env python3
"""Downscale PNG textures in place."""

from pathlib import Path
from PIL import Image

textures = Path(__file__).resolve().parent.parent / "content" / "textures"
files = sorted(path for path in textures.rglob("*") if path.is_file() and path.suffix.lower() == ".png")

for path in files:
    with Image.open(path) as image:
        if image.mode not in ("RGB", "RGBA"):
            raise ValueError(f"Expected RGB or RGBA image, got {image.mode}: {path}")
        width, height = image.size
        size = (max(1, (width + 1) // 2), max(1, (height + 1) // 2))
        resized = image.resize(size, Image.Resampling.LANCZOS)
        options = {
            key: image.info[key]
            for key in ("icc_profile", "exif", "dpi")
            if key in image.info
        }
        resized.save(path, **options)
    print(
        f"{path.relative_to(textures)}: {width}x{height} -> {size[0]}x{size[1]}"
    )
print(f"Downscaled {len(files)} texture(s).")
