// Project Nuclidean Source File

#include <engine/editor/editor_renderer.h>
#include <engine/editor/editor.h>
#include <engine/editor/rendering_modifier.h>

#include <common.h>

namespace nc
{

//==================================================================================================
// Restores the GL state when exiting the scope.
class OpenGlStateScope
{
public:
  OpenGlStateScope()
  {
    m_depth_test  = glIsEnabled(GL_DEPTH_TEST);
    m_alpha_blend = glIsEnabled(GL_BLEND);
    glGetFloatv(GL_LINE_WIDTH, &m_line_width);
  }

  ~OpenGlStateScope()
  {
    auto reset_state = [](auto flag, bool state)
    {
      if (state)
        glEnable(flag);
      else
        glDisable(flag);
    };

    reset_state(GL_DEPTH_TEST, m_depth_test);
    reset_state(GL_BLEND,      m_alpha_blend);
    glLineWidth(m_line_width);
  }

private:
  bool m_depth_test  = false;
  bool m_alpha_blend = false;
  f32  m_line_width  = 0.0f;
};

//==================================================================================================
EditorRenderer::EditorRenderer()
: grid_rendering
(
  ShaderProgramHandle::from_files(shaders::editor::lines::VERTEX_FILE, shaders::editor::lines::FRAGMENT_FILE)
)
{
  nc_assert(grid_rendering.is_valid());
}

//==================================================================================================
void EditorRenderer::render
(
  mat3                                          projection,
  const std::span<EditorPrimitivePtr>           primitives,
  std::span<IEditorPrimitiveRenderingModifier*> modifiers
)
{
  glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Editor pass");
  OpenGlStateScope state_restore;

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);

  grid_rendering.use();
  grid_rendering.set_uniform(shaders::editor::lines::TRANSFORM, projection);

  for (const EditorPrimitivePtr& primitive : primitives)
  {
    EditorPrimitiveRenderingProperties properties_override = primitive->properties;
    for (auto* modifier : modifiers)
    {
      modifier->modify_rendering_properties(properties_override, *primitive);
    }

    nc_assert(primitive->is_valid());
    grid_rendering.set_uniform(shaders::editor::lines::COLOR, properties_override.color);
    glLineWidth(properties_override.line_width);
    glBindVertexArray(primitive->handle.get_vao());
    glDrawArrays(primitive->handle.get_draw_mode(), 0, primitive->handle.get_vertex_count());
  }

  glBindVertexArray(0);
  glPopDebugGroup();
}

}
