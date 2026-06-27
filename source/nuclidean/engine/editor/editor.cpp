// Project Nuclidean Source File
#pragma once

#include <engine/editor/editor.h>

#if NC_EDITOR

#include <engine/input/input_system.h>
#include <engine/editor/editor_system.h>

#include <engine/graphics/shaders/uniform.h>
#include <engine/graphics/shaders/shaders.h>
#include <engine/graphics/resources/shader_program.h>
#include <engine/graphics/resources/mesh.h>

#include <imgui/imgui.h>

#include <vector>
#include <span>

namespace nc
{

//==================================================================================================
// World coords to screen coords.
[[maybe_unused]] static mat3 calc_view_matrix(vec2 offset, f32 zoom, f32 aspect)
{
  vec3 c0  = vec3{1.0f, 0.0f, 0.0f} * zoom;
  vec3 c1  = vec3{0.0f, 1.0f, 0.0f} * zoom * aspect;
  vec3 c2  = vec3{-offset, 1.0f};
  mat3 mat = {c0, c1, c2};
  return mat;
}

//==================================================================================================
struct EditorPrimitive
{
  MeshHandle handle     = MeshHandle::invalid();
  aabb2      bbox       = aabb2{};
  f32        line_width = 1.0f;
  color4     color      = colors::WHITE;

  void refresh_gpu_data(std::span<vec2> points)
  {
    // Refresh the bbox
    bbox = aabb2{};
    for (vec2 p : points)
    {
      bbox.insert_point(p);
    }

    // Upload the data
    MeshManager::get().destroy_editor_primitive(handle);
    handle = MeshManager::get().create_editor_primitive(points, GL_LINES);
  }

  ~EditorPrimitive()
  {
    // It should be ok to call this even if the handle is null
    MeshManager::get().destroy_editor_primitive(handle);
  }
};

//==================================================================================================
// Restores the GL state when exiting the scope.
class OpenGlStateScope
{
public:
  OpenGlStateScope()
  {
    m_depth_test = glIsEnabled(GL_DEPTH_TEST);
    glGetFloatv(GL_LINE_WIDTH, &m_line_width);
  }

  ~OpenGlStateScope()
  {
    if (m_depth_test)
      glEnable(GL_DEPTH_TEST);
    else
      glDisable(GL_DEPTH_TEST);

    glLineWidth(m_line_width);
  }

private:
  bool m_depth_test = false;
  f32  m_line_width = 0.0f;
};

//==================================================================================================
class EditorRenderer
{
public:
  EditorRenderer()
  : grid_rendering(ShaderProgramHandle::from_files(shaders::editor::lines::VERTEX_FILE, shaders::editor::lines::FRAGMENT_FILE))
  {
    nc_assert(grid_rendering.is_valid());
  }

  void render(mat3 projection, const std::span<EditorPrimitive*> primitives)
  {
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Editor pass");
    OpenGlStateScope state_restore;

    grid_rendering.use();
    grid_rendering.set_uniform(shaders::editor::lines::TRANSFORM, projection);

    for (const EditorPrimitive* primitive : primitives)
    {
      grid_rendering.set_uniform(shaders::editor::lines::COLOR, primitive->color);
      glLineWidth(primitive->line_width);
      glBindVertexArray(primitive->handle.get_vao());
      glDrawArrays(primitive->handle.get_draw_mode(), 0, primitive->handle.get_vertex_count());
    }

    glBindVertexArray(0);
    glPopDebugGroup();
  }

private:
  ShaderProgramHandle grid_rendering;
};

//==================================================================================================
struct EditorWall
{
  vec2 pt;
};

//==================================================================================================
struct EditorSector
{
  EditorPrimitive         render_data;
  std::vector<EditorWall> walls;
  f32                     floor_height = 0.0f;
  f32                     ceil_height  = 0.0f;
};

//==================================================================================================
struct Editor::EditorImpl
{
  f32                       aspect = 1.0f;
  EditorPrimitive           grid;
  std::vector<EditorSector> sectors;
  EditorRenderer            renderer;
  vec2                      center = VEC2_ZERO;
  f32                       zoom   = 1.0f;

  bool is_dragging                     = false;
  vec2 dragging_start_cursor_world_pos = VEC2_ZERO;
  vec2 dragging_start_center_world_pos = VEC2_ZERO;

  void init()
  {
    std::vector<vec2> points;
    points.insert(points.begin(), {vec2{-1.0f,  0.0f}, vec2{1.0f, 0.0f}});
    points.insert(points.begin(), {vec2{ 0.0f, -1.0f}, vec2{0.0f, 1.0f}});
    grid.refresh_gpu_data(points);
    grid.color = colors::GRAY;
    grid.line_width = 1.0f;
  }

  vec2 screen_to_wpos(vec2 screen_pos)
  {
    mat3 screen_to_world = calc_view_matrix(this->center, this->zoom, this->aspect);
    return (screen_to_world * vec3{screen_pos, 1.0f}).xy;
  }

  vec2 get_mouse_normalized()
  {
    ImVec2 mouse   = ImGui::GetMousePos();
    ImVec2 display = ImGui::GetIO().DisplaySize;
    return {mouse.x / display.x, mouse.y / display.y};
  }

  // Returns mouse coords in [-1, 1] range.
  // [-1, -1] is bottom left.
  vec2 get_mouse_screen_pos()
  {
    vec2 mpos_screen_space = this->get_mouse_normalized();
    vec2 mpos_normalized   = (mpos_screen_space * 2.0f - VEC2_ONE) * vec2{1.0f, -1.0f};
    return mpos_normalized;
  }

  vec2 get_mouse_wpos()
  {
    vec2 mpos = this->get_mouse_screen_pos();
    return screen_to_wpos(mpos);
  }

  void handle_dragging()
  {
    bool middle_mouse = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

    if (middle_mouse != is_dragging)
    {
      is_dragging = middle_mouse;
      if (is_dragging)
      {
        // Started dragging
        dragging_start_center_world_pos = this->center;
        dragging_start_cursor_world_pos = this->get_mouse_wpos();
      }
      else
      {
        // Ended..
      }
    }

    if (is_dragging)
    {
      vec2 current_mouse_wpos    = this->get_mouse_wpos();
      vec2 difference_from_start = this->dragging_start_cursor_world_pos - current_mouse_wpos;
      this->center = this->dragging_start_center_world_pos + difference_from_start;
    }
  }
};

//==================================================================================================
/*static*/ Editor* Editor::get()
{
  return EditorSystem::get().get_editor();
}

//==================================================================================================
Editor::Editor()  = default;
Editor::~Editor() = default;

//==================================================================================================
void Editor::init()
{
  InputSystem::get().lock_player_input(InputLockLayers::editor, true);
  nc_assert(!m_impl);
  m_impl = std::make_unique<EditorImpl>();
  m_impl->init();
}

//==================================================================================================
void Editor::terminate()
{
  nc_assert(m_impl);
  m_impl.reset();
  InputSystem::get().lock_player_input(InputLockLayers::editor, false);
}

//==================================================================================================
void Editor::update(f32 /*delta*/)
{
  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      ImGui::MenuItem("Save Map",    "Ctrl+S");
      ImGui::MenuItem("Save Map As", "Ctrl+Shift+S");
      ImGui::MenuItem("Open Map",    "Ctrl+O");

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  m_impl->handle_dragging();
}

//==================================================================================================
void Editor::render()
{
  std::vector<EditorPrimitive*> primitives;

  primitives.push_back(&m_impl->grid);

  for (EditorSector& sector : m_impl->sectors)
  {
    if (sector.render_data.handle.is_valid())
    {
      primitives.push_back(&sector.render_data);
    }
  }

  mat3 projection = calc_view_matrix(m_impl->center, m_impl->zoom, m_impl->aspect);
  m_impl->renderer.render(projection, primitives);
}

//==================================================================================================
void Editor::on_window_resized(u32 width, u32 height)
{
  m_impl->aspect = cast<f32>(width) / height;
}

}

#endif // #if NC_EDITOR
