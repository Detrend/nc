// Project Nuclidean Source File
#pragma once

#include <engine/graphics/gl_types.h>
#include <engine/graphics/resources/shader_program.h>

namespace nc
{

struct UiDrawList;

// Temporary renderer for UI until we have the new UI system
class UiRenderer
{
public:
  UiRenderer();
  ~UiRenderer();

  UiRenderer(const UiRenderer&)            = delete;
  UiRenderer& operator=(const UiRenderer&) = delete;

  // Draws the quads in order on top of whatever is in the framebuffer.
  void draw(const UiDrawList& draw_list) const;

private:
  static constexpr u32 UV_LAYOUT_COUNT = 2; // see UiQuadUvs

  ShaderProgramHandle m_image_material;
  ShaderProgramHandle m_glyph_material;

  GLuint m_vaos[UV_LAYOUT_COUNT] = {};
  GLuint m_vbos[UV_LAYOUT_COUNT] = {};
};

}
