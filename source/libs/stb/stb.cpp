#include <stddef.h>

#define stbi_default_alignment 16

namespace nc {
  void* scratch_malloc(const size_t size_to_alloc, const size_t alignment);
  void* scratch_realloc(void* const ptr, const size_t old_size, const size_t new_size, const size_t alignment);
  void  scratch_free(void* const ptr);
}


#define STBI_MALLOC(sz)                       ::nc::scratch_malloc(sz, stbi_default_alignment)
#define STBI_REALLOC_SIZED(p,oldsz,newsz)     ::nc::scratch_realloc(p,oldsz,newsz, stbi_default_alignment) 
#define STBI_FREE(p)                          ::nc::scratch_free(p)



#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"