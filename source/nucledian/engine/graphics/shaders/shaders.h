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
      #include <engine/graphics/shaders/solid.vert>
      #include <engine/graphics/shaders/solid.frag>

      inline constexpr Uniform<0, mat4>   TRANSFORM;
      inline constexpr Uniform<1, mat4>   VIEW;
      inline constexpr Uniform<2, mat4>   PROJECTION;
      inline constexpr Uniform<3, color4> COLOR;
      inline constexpr Uniform<4, bool>   UNLIT;
    }

    namespace billboard
    {
      #include <engine/graphics/shaders/billboard.vert>
      #include <engine/graphics/shaders/billboard.frag>

      inline constexpr Uniform<0, mat4> TRANSFORM;
      inline constexpr Uniform<1, mat4> VIEW;
      inline constexpr Uniform<2, mat4> PROJECTION;
      inline constexpr Uniform<3, s32>  TEXTURE;

      inline constexpr Uniform<4, vec2> ATLAS_SIZE;
      inline constexpr Uniform<5, vec2> TEXTURE_POS;
      inline constexpr Uniform<6, vec2> TEXTURE_SIZE;
    }

    // Lighting pass.
    namespace light
    {
      #include <engine/graphics/shaders/light.vert>
      #include <engine/graphics/shaders/light.frag>

      inline constexpr Uniform<0, s32>  G_POSITION;
      inline constexpr Uniform<1, s32>  G_NORMAL;
      inline constexpr Uniform<2, s32>  G_ALBEDO;
      inline constexpr Uniform<3, vec3> VIEW_POSITION;
    }

    // Sector rendering.
    namespace sector
    {
      #include <engine/graphics/shaders/sector.vert>
      #include <engine/graphics/shaders/sector.frag>

      inline constexpr Uniform<0, mat4> VIEW;
      inline constexpr Uniform<1, mat4> PROJECTION;
      inline constexpr Uniform<2, vec2> GAME_ATLAS_SIZE;
      inline constexpr Uniform<3, vec2> LEVEL_ATLAS_SIZE;
      inline constexpr Uniform<4, s32>  GAME_TEXTURE_ATLAS;
      inline constexpr Uniform<5, s32>  LEVEL_TEXTURE_ATLAS;
    }

    namespace ui_button
    {
      #include <engine/ui/ui_button.vert>
      #include <engine/ui/ui_button.frag>

      inline constexpr Uniform<0, mat4> TRANSFORM;

      inline constexpr Uniform<1, vec2> ATLAS_SIZE;
      inline constexpr Uniform<2, vec2> TEXTURE_POS;
      inline constexpr Uniform<3, vec2> TEXTURE_SIZE;
    }
  }

}
