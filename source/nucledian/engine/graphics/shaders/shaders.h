#pragma once

#include <types.h>
#include <math/vector.h>
#include <math/matrix.h>

#include <engine/graphics/gl_types.h>
#include <engine/graphics/resources/texture.h>


#include <glad/glad.h>

namespace nc
{
  namespace shaders
  {
    // Solid geometry.
    namespace solid
    {
      inline constexpr const char* VERTEX_FILE   = "engine/graphics/shaders/solid.vert";
      inline constexpr const char* FRAGMENT_FILE = "engine/graphics/shaders/solid.frag";

      inline constexpr Uniform<0, mat4>   TRANSFORM;
      inline constexpr Uniform<1, mat4>   VIEW;
      inline constexpr Uniform<2, mat4>   PROJECTION;
      inline constexpr Uniform<3, color4> COLOR;
      inline constexpr Uniform<4, bool>   UNLIT;
    }

    namespace billboard
    {
      inline constexpr const char* VERTEX_FILE   = "engine/graphics/shaders/billboard.vert";
      inline constexpr const char* FRAGMENT_FILE = "engine/graphics/shaders/billboard.frag";

      inline constexpr Uniform<0, mat4> TRANSFORM;
      inline constexpr Uniform<1, mat4> VIEW;
      inline constexpr Uniform<2, mat4> PROJECTION;
      inline constexpr Uniform<3, vec2> ATLAS_SIZE;
      inline constexpr Uniform<4, vec2> TEXTURE_POS;
      inline constexpr Uniform<5, vec2> TEXTURE_SIZE;
      inline constexpr Uniform<6, u32>  SECTOR_ID;
      inline constexpr Uniform<7, mat4> PORTAL_DEST_TO_SRC;
      inline constexpr Uniform<8, u32>  MATRIX_ID;
      inline constexpr Uniform<9, bool> ENABLE_SHADOWS;
    }

    namespace gun
    {
      inline constexpr const char* VERTEX_FILE   = "engine/graphics/shaders/gun.vert";
      inline constexpr const char* FRAGMENT_FILE = "engine/graphics/shaders/billboard.frag";

      inline constexpr Uniform<0, mat4> TRANSFORM;
      inline constexpr Uniform<1, mat4> VIEW;
      inline constexpr Uniform<2, mat4> PROJECTION;
      inline constexpr Uniform<3, vec2> ATLAS_SIZE;
      inline constexpr Uniform<4, vec2> TEXTURE_POS;
      inline constexpr Uniform<5, vec2> TEXTURE_SIZE;
      inline constexpr Uniform<6, u32>  SECTOR_ID;
      inline constexpr Uniform<7, mat4> PORTAL_DEST_TO_SRC;
      inline constexpr Uniform<8, u32>  MATRIX_ID;
      inline constexpr Uniform<9, bool> ENABLE_SHADOWS;
    }

    // Lighting pass.
    namespace light
    {
      inline constexpr const char* VERTEX_FILE   = "engine/graphics/shaders/light.vert";
      inline constexpr const char* FRAGMENT_FILE = "engine/graphics/shaders/light.frag";

      inline constexpr Uniform<0, vec3> VIEW_POSITION;
      inline constexpr Uniform<1, u32>  NUM_DIR_LIGHTS;
      inline constexpr Uniform<2, u32>  NUM_TILES_X;
      inline constexpr Uniform<3, f32>  AMBIENT_STRENGTH;
      inline constexpr Uniform<4, u32>  NUM_SECTORS;
      inline constexpr Uniform<5, u32>  NUM_WALLS;
    }

    // Pixel lighting pass.
    namespace pixel_light
    {
      inline constexpr const char* VERTEX_FILE   = "engine/graphics/shaders/pixel_light.vert";
      inline constexpr const char* FRAGMENT_FILE = "engine/graphics/shaders/pixel_light.frag";

      inline constexpr Uniform<0,  vec2> FROM;
      inline constexpr Uniform<1,  vec2> TO;
      inline constexpr Uniform<2,  vec2> MEGATEX_SIZE;
      inline constexpr Uniform<3,  vec3> COLOR;
      inline constexpr Uniform<4,  vec3> WP00;
      inline constexpr Uniform<5,  vec3> WP10;
      inline constexpr Uniform<6,  vec3> WP01;
      inline constexpr Uniform<7,  vec3> WP11;
      inline constexpr Uniform<8,  u32>  NUM_LIGHTS;
      inline constexpr Uniform<9,  vec3> NORMAL;
      inline constexpr Uniform<10, u32>  NUM_SECTORS;
      inline constexpr Uniform<11, u32>  NUM_WALLS;
      inline constexpr Uniform<12, u32>  SECTOR_ID;
      inline constexpr Uniform<13, f32>  AMBIENT_STRENGTH;
      inline constexpr Uniform<14, vec3> CAMERA_POS;
    }

    // Pixel GI lighting pass
    namespace pixel_gi
    {
      inline constexpr const char* VERTEX_FILE   = "engine/graphics/shaders/pixel_gi.vert";
      inline constexpr const char* FRAGMENT_FILE = "engine/graphics/shaders/pixel_gi.frag";

      inline constexpr Uniform<0,  vec2> FROM;
      inline constexpr Uniform<1,  vec2> TO;
      inline constexpr Uniform<2,  vec3> WP00;
      inline constexpr Uniform<3,  vec3> WP10;
      inline constexpr Uniform<4,  vec3> WP01;
      inline constexpr Uniform<5,  vec3> WP11;
      inline constexpr Uniform<6,  vec3> NORMAL;
      inline constexpr Uniform<7,  vec2> MEGATEX_SIZE;
      inline constexpr Uniform<8,  u32>  NUM_INDICES;
      inline constexpr Uniform<9,  u32>  MY_PART_ID;
      inline constexpr Uniform<10, vec2> GAME_ATLAS_SIZE;
    }

    // Pixel GI lighting pass
    namespace pixel_denoise
    {
      inline constexpr const char* VERTEX_FILE   = "engine/graphics/shaders/pixel_denoise.vert";
      inline constexpr const char* FRAGMENT_FILE = "engine/graphics/shaders/pixel_denoise.frag";

      inline constexpr Uniform<0,  vec2> FROM;
      inline constexpr Uniform<1,  vec2> TO;
      inline constexpr Uniform<2,  vec3> WP00;
      inline constexpr Uniform<3,  vec3> WP10;
      inline constexpr Uniform<4,  vec3> WP01;
      inline constexpr Uniform<5,  vec3> WP11;
      inline constexpr Uniform<7,  vec2> MEGATEX_SIZE;
    }

    // Sector rendering.
    namespace sector
    {
      inline constexpr const char* VERTEX_FILE   = "engine/graphics/shaders/sector.vert";
      inline constexpr const char* FRAGMENT_FILE = "engine/graphics/shaders/sector.frag";

      inline constexpr Uniform<0, mat4> VIEW;
      inline constexpr Uniform<1, mat4> PROJECTION;
      inline constexpr Uniform<2, vec2> GAME_ATLAS_SIZE;
      inline constexpr Uniform<3, vec2> LEVEL_ATLAS_SIZE;
      inline constexpr Uniform<4, mat4> PORTAL_DEST_TO_SRC;
      inline constexpr Uniform<5, u32>  SECTOR_ID;
      inline constexpr Uniform<6, u32>  MATRIX_ID;
    }

    namespace light_culling
    {
      inline constexpr const char* COMPUTE_FILE = "engine/graphics/shaders/light_culling.comp";

      inline constexpr Uniform<0, mat4> VIEW;
      inline constexpr Uniform<1, mat4> INV_PROJECTION;
      inline constexpr Uniform<2, vec2> WINDOW_SIZE;
      inline constexpr Uniform<3, f32>  FAR_PLANE;
      inline constexpr Uniform<4, u32>  NUM_LIGHTS;
    }

    namespace ui_button
    {
      inline constexpr const char* VERTEX_FILE   = "engine/ui/ui_button.vert";
      inline constexpr const char* FRAGMENT_FILE = "engine/ui/ui_button.frag";

      inline constexpr Uniform<0, mat4> TRANSFORM;

      inline constexpr Uniform<1, vec2> ATLAS_SIZE;
      inline constexpr Uniform<2, vec2> TEXTURE_POS;
      inline constexpr Uniform<3, vec2> TEXTURE_SIZE;
      inline constexpr Uniform<4, bool> HOVER;
      inline constexpr Uniform<5, vec4> COLOR;
    }

    namespace ui_text
    {
      inline constexpr const char* VERTEX_FILE   = "engine/ui/ui_text.vert";
      inline constexpr const char* FRAGMENT_FILE = "engine/ui/ui_text.frag";

      inline constexpr Uniform<0, mat4> TRANSFORM;

      inline constexpr Uniform<1, vec2> ATLAS_SIZE;
      inline constexpr Uniform<2, vec2> TEXTURE_POS;
      inline constexpr Uniform<3, vec2> TEXTURE_SIZE;
      inline constexpr Uniform<4, s32> CHARACTER;
      inline constexpr Uniform<5, s32> WIDTH;
      inline constexpr Uniform<6, s32> HEIGHT;

    }

    namespace sky_box
    {
      inline constexpr const char* VERTEX_FILE   = "engine/graphics/shaders/sky_box.vert";
      inline constexpr const char* FRAGMENT_FILE = "engine/graphics/shaders/sky_box.frag";

      inline constexpr Uniform<0, mat4> VIEW;
      inline constexpr Uniform<1, mat4> PROJECTION;
      inline constexpr Uniform<2, f32>  EXPOSURE;
      inline constexpr Uniform<3, bool> USE_GAMMA_CORRECTION;
    }
  }

}
