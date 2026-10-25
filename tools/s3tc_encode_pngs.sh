#!/bin/sh
# apt install libnvtt-bin

usage() {
    printf 'Usage: %s DIRECTORY\n' "${0##*/}" >&2
    printf 'Recursively convert atlas PNG files to format-appropriate DDS files.\n' >&2
}

if [ "$#" -ne 1 ]; then
    usage
    exit 2
fi

input_dir=$1
nvcompress=${NVCOMPRESS:-nvcompress}

if [ ! -d "$input_dir" ]; then
    printf 'Error: not a directory: %s\n' "$input_dir" >&2
    exit 2
fi

if ! command -v "$nvcompress" >/dev/null 2>&1; then
    printf 'Error: nvcompress executable not found: %s (install package libnvtt-bin)\n' "$nvcompress" >&2
    exit 127
fi

if find "$input_dir" -type f -exec sh -c '
    nvcompress=$1
    shift

    for input do
        case ${input##*/} in
            *.[Pp][Nn][Gg]) ;;
            *) continue ;;
        esac

        output=${input%.*}.dds
        case ${input##*/} in
            diffuse_*) format=BC3; option=-bc3 ;;
            normal_*) format=BC1; option=-bc1 ;;
            specular_*|emissive_*) format=BC4; option=-bc4 ;;
            *)
                printf "Error: cannot determine texture format from filename: %s\n" "$input" >&2
                exit 2
                ;;
        esac
        printf "Encoding %s -> %s (%s)\n" "$input" "$output" "$format"
        if "$nvcompress" -nomips "$option" "$input" "$output"; then
            :
        else
            status=$?
            exit "$status"
        fi
    done
' sh "$nvcompress" {} +
then
    printf 'Finished processing PNG files.\n'
else
    status=$?
    exit "$status"
fi
