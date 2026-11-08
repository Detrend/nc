// Project Nuclidean Source File
#include <engine/graphics/ui_draw_list.h>
#include <engine/graphics/resources/texture.h>

namespace nc
{

//==============================================================================
static UiQuad make_quad(const TextureHandle& texture, vec2 position, vec2 scale, bool hover)
{
  const TextureAtlasBundle& atlas = texture.get_atlas_bundle();

  return UiQuad
  {
    .position      = position,
    .scale         = scale,
    .atlas_size    = atlas.get_size(),
    .texture_pos   = texture.get_pos(),
    .texture_size  = texture.get_size(),
    .atlas_texture = atlas.diffuse_handle,
    .hover         = hover,
  };
}

//==============================================================================
UiQuad& UiDrawList::add_image
(
  const TextureHandle& texture,
  vec2                 position,
  vec2                 scale,
  bool                 hover
)
{
  return quads.emplace_back(make_quad(texture, position, scale, hover));
}

//==============================================================================
UiQuad& UiDrawList::add_glyph
(
  const TextureHandle& texture,
  vec2                 position,
  vec2                 scale,
  s32                  character,
  s32                  grid_width,
  s32                  grid_height,
  bool                 hover
)
{
  UiQuad& quad     = quads.emplace_back(make_quad(texture, position, scale, hover));
  quad.shader      = UiQuadShader::glyph;
  quad.character   = character;
  quad.grid_width  = grid_width;
  quad.grid_height = grid_height;
  return quad;
}

//==============================================================================
UiQuad& UiDrawList::add_char
(
  const TextureHandle& font,
  vec2                 position,
  vec2                 scale,
  s32                  character,
  bool                 hover
)
{
  constexpr s32 FONT_GRID_WIDTH  = 8;
  constexpr s32 FONT_GRID_HEIGHT = 16;

  return add_glyph(font, position, scale, character, FONT_GRID_WIDTH, FONT_GRID_HEIGHT, hover);
}

}
