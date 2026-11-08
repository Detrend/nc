// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <math/vector.h>
#include <engine/graphics/gl_types.h>

#include <vector>

namespace nc
{

class TextureHandle;

// Which shader is used to draw the quad.
enum class UiQuadShader : u8
{
  image, // ui_button shader, draws a whole texture
  glyph, // ui_text shader, draws one cell of a texture laid out as a grid
};

// Which UV layout is used for the quad.
enum class UiQuadUvs : u8
{
  inset, // UVs slightly inset vertically, used by almost the whole UI
  full,  // UVs covering the whole texture, used by the screen effect
};

struct UiQuad
{
  vec2         position      = VEC2_ZERO; // center in normalized device coordinates
  vec2         scale         = VEC2_ONE;  // half extents in normalized device coordinates
  vec2         atlas_size    = VEC2_ONE;
  vec2         texture_pos   = VEC2_ZERO;
  vec2         texture_size  = VEC2_ZERO;
  vec4         color         = VEC4_ONE;  // image shader only
  GLuint       atlas_texture = 0;
  s32          character     = 0;         // glyph shader only, index of the grid cell
  s32          grid_width    = 0;         // glyph shader only
  s32          grid_height   = 0;         // glyph shader only
  UiQuadShader shader        = UiQuadShader::image;
  UiQuadUvs    uvs           = UiQuadUvs::inset;
  bool         hover         = false;
};

struct UiDrawList
{
  UiQuad& add_image
  (
    const TextureHandle& texture,
    vec2                 position,
    vec2                 scale,
    bool                 hover = false
  );

  UiQuad& add_glyph
  (
    const TextureHandle& texture,
    vec2                 position,
    vec2                 scale,
    s32                  character,
    s32                  grid_width,
    s32                  grid_height,
    bool                 hover = false
  );

  UiQuad& add_char
  (
    const TextureHandle& font,
    vec2                 position,
    vec2                 scale,
    s32                  character,
    bool                 hover = false
  );

  std::vector<UiQuad> quads;
};

}
