// Project Nuclidean Source File
#include <engine/graphics/ui_renderer.h>
#include <engine/graphics/ui_draw_list.h>
#include <engine/graphics/shaders/shaders.h>

#include <common.h>

#include <glm/gtc/matrix_transform.hpp>

namespace nc
{

//==============================================================================
static void create_quad(GLuint& vao, GLuint& vbo, UiQuadUvs uvs)
{
  // Most of the UI insets the UVs a bit vertically to avoid bleeding of the
  // neighbouring atlas texels, the screen effect covers the whole texture.
  f32 v_inset = (uvs == UiQuadUvs::inset) ? 0.015f : 0.0f;

  // position, uv
  vec2 vertices[] =
  {
    vec2(-1.0f,  1.0f), vec2(0.0f, v_inset),
    vec2(-1.0f, -1.0f), vec2(0.0f, 1.0f - v_inset),
    vec2( 1.0f,  1.0f), vec2(1.0f, v_inset),
    vec2( 1.0f, -1.0f), vec2(1.0f, 1.0f - v_inset),
  };

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // position
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0);
  glEnableVertexAttribArray(0);
  // uvs
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), recast<void*>(2 * sizeof(f32)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//==============================================================================
UiRenderer::UiRenderer()
: m_image_material(ShaderProgramHandle::from_files(shaders::ui_button::VERTEX_FILE, shaders::ui_button::FRAGMENT_FILE))
, m_glyph_material(ShaderProgramHandle::from_files(shaders::ui_text::VERTEX_FILE,   shaders::ui_text::FRAGMENT_FILE))
{
  create_quad(m_vaos[cast<u32>(UiQuadUvs::inset)], m_vbos[cast<u32>(UiQuadUvs::inset)], UiQuadUvs::inset);
  create_quad(m_vaos[cast<u32>(UiQuadUvs::full)],  m_vbos[cast<u32>(UiQuadUvs::full)],  UiQuadUvs::full);
}

//==============================================================================
UiRenderer::~UiRenderer()
{
  glDeleteVertexArrays(UV_LAYOUT_COUNT, m_vaos);
  glDeleteBuffers(UV_LAYOUT_COUNT, m_vbos);
}

//==============================================================================
void UiRenderer::draw(const UiDrawList& draw_list) const
{
  if (draw_list.quads.empty())
  {
    return;
  }

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glActiveTexture(GL_TEXTURE0);

  const ShaderProgramHandle* bound_material = nullptr;
  GLuint                     bound_vao      = 0;

  for (const UiQuad& quad : draw_list.quads)
  {
    const ShaderProgramHandle& material = (quad.shader == UiQuadShader::image)
      ? m_image_material
      : m_glyph_material;

    if (bound_material != &material)
    {
      material.use();
      bound_material = &material;
    }

    GLuint vao = m_vaos[cast<u32>(quad.uvs)];
    if (bound_vao != vao)
    {
      glBindVertexArray(vao);
      bound_vao = vao;
    }

    mat4 transform = glm::translate(mat4(1.0f), vec3(quad.position.x, quad.position.y, 0.0f));
    transform      = glm::scale(transform, vec3(quad.scale.x, quad.scale.y, 1.0f));

    if (quad.shader == UiQuadShader::image)
    {
      material.set_uniform(shaders::ui_button::TRANSFORM,    transform);
      material.set_uniform(shaders::ui_button::ATLAS_SIZE,   quad.atlas_size);
      material.set_uniform(shaders::ui_button::TEXTURE_POS,  quad.texture_pos);
      material.set_uniform(shaders::ui_button::TEXTURE_SIZE, quad.texture_size);
      material.set_uniform(shaders::ui_button::HOVER,        quad.hover);
      material.set_uniform(shaders::ui_button::COLOR,        quad.color);
    }
    else
    {
      material.set_uniform(shaders::ui_text::TRANSFORM,    transform);
      material.set_uniform(shaders::ui_text::ATLAS_SIZE,   quad.atlas_size);
      material.set_uniform(shaders::ui_text::TEXTURE_POS,  quad.texture_pos);
      material.set_uniform(shaders::ui_text::TEXTURE_SIZE, quad.texture_size);
      material.set_uniform(shaders::ui_text::CHARACTER,    quad.character);
      material.set_uniform(shaders::ui_text::WIDTH,        quad.grid_width);
      material.set_uniform(shaders::ui_text::HEIGHT,       quad.grid_height);
      material.set_uniform(shaders::ui_text::HOVER,        quad.hover);
    }

    glBindTexture(GL_TEXTURE_2D, quad.atlas_texture);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
}

}
