#!/bin/sh
set -eu

usage() {
    printf 'Usage: %s DIRECTORY [BLOCK_SIZE]\n' "${0##*/}" >&2
    printf 'Recursively convert PNG files to ASTC (default block size: 6x6).\n' >&2
}

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    usage
    exit 2
fi

input_dir=$1
block_size=${2:-6x6}
astcenc=${ASTCENC:-astcenc}

if [ ! -d "$input_dir" ]; then
    printf 'Error: not a directory: %s\n' "$input_dir" >&2
    exit 2
fi

case "$block_size" in
    *[!0-9x]*|x*|*x|*x*x*)
        printf 'Error: invalid ASTC block size: %s\n' "$block_size" >&2
        exit 2
        ;;
esac

if ! command -v "$astcenc" >/dev/null 2>&1; then
    printf 'Error: astcenc executable not found: %s\n' "$astcenc" >&2
    exit 127
fi

find "$input_dir" -type f -iname '*.png' -exec sh -c '
    block_size=$1
    astcenc=$2
    shift 2

    for input do
        output=${input%.*}.astc
        printf "Encoding %s -> %s\n" "$input" "$output"
        "$astcenc" -cl "$input" "$output" "$block_size" -exhaustive
    done
' sh "$block_size" "$astcenc" {} +
printf 'Finished processing PNG files.\n'
