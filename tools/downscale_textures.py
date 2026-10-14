#!/usr/bin/env python3
"""Create half-width, half-height copies of the engine's PNG textures.

By default, reads content/textures and writes a mirrored tree to
content/textures_half, leaving the originals untouched. Install Pillow with
`python -m pip install Pillow`.
"""

import argparse
import os
import tempfile
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:
    raise SystemExit(
        "Pillow is required. Install it with: python -m pip install Pillow"
    ) from exc


SUPPORTED_EXTENSIONS = {".png"}


def downscale(source: Path, destination: Path) -> tuple[int, int, int, int]:
    with Image.open(source) as image:
        width, height = image.size
        size = (max(1, (width + 1) // 2), max(1, (height + 1) // 2))
        if image.mode == "P":
            image = image.convert("RGBA" if "transparency" in image.info else "RGB")
        elif image.mode == "1":
            image = image.convert("L")
        resized = image.resize(size, Image.Resampling.LANCZOS)

        save_options = {}
        for key in ("icc_profile", "exif", "dpi"):
            if key in image.info:
                save_options[key] = image.info[key]

        destination.parent.mkdir(parents=True, exist_ok=True)
        suffix = destination.suffix
        with tempfile.NamedTemporaryFile(
            dir=destination.parent, suffix=suffix, delete=False
        ) as temporary:
            temporary_path = Path(temporary.name)
        try:
            resized.save(temporary_path, **save_options)
            os.replace(temporary_path, destination)
        finally:
            temporary_path.unlink(missing_ok=True)

        return width, height, size[0], size[1]


def main() -> None:
    project_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(
        description="Downscale each PNG texture to about one quarter its pixel count."
    )
    parser.add_argument(
        "--source",
        type=Path,
        default=project_root / "content" / "textures",
        help="texture directory to scan (default: content/textures)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="output directory (default: content/textures_half)",
    )
    parser.add_argument(
        "--in-place",
        action="store_true",
        help="replace source images instead of writing copies",
    )
    args = parser.parse_args()

    source = args.source.resolve()
    if not source.is_dir():
        parser.error(f"source directory does not exist: {source}")
    if args.in_place and args.output is not None:
        parser.error("--output cannot be used with --in-place")

    output = None if args.in_place else (
        args.output.resolve() if args.output else project_root / "content" / "textures_half"
    )
    if output is not None and (output == source or source in output.parents):
        parser.error("output directory must not be inside the source directory")

    files = sorted(
        path
        for path in source.rglob("*")
        if path.is_file()
        and path.suffix.lower() in SUPPORTED_EXTENSIONS
    )
    if not files:
        print(f"No PNG textures found in {source}")
        return

    for path in files:
        destination = path if output is None else output / path.relative_to(source)
        old_width, old_height, new_width, new_height = downscale(path, destination)
        print(
            f"{path.relative_to(source)}: "
            f"{old_width}x{old_height} -> {new_width}x{new_height}"
        )

    print(f"Downscaled {len(files)} texture(s).")
    if output is not None:
        print(f"Output: {output}")
    print("Only PNG files are processed.")


if __name__ == "__main__":
    main()
