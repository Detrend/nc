#include <config.h>

#ifdef NC_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-but-set-variable"

#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#ifdef NC_COMPILER_CLANG
#pragma clang diagnostic pop
#endif