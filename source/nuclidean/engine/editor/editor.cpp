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

#include <engine/graphics/resources/texture.h>

#include <math/lingebra.h>   // compMax
#include <metaprogramming.h> // ARRAY_LENGTH

#include <imgui/imgui.h>

#include <algorithm> // std::transform
#include <vector>
#include <span>
#include <optional>
#include <variant>
#include <memory>

//==================================================================================================
namespace nc::editor
{

// Default alpha of the grid.
constexpr f32 GRID_ALPHA = 0.3f;

// Width/height of one grid cell has to occupy at least this amount of screen in order to be visible
constexpr f32 GRID_SCREEN_PERCENTAGE_FOR_VISIBILITY = 0.01f;

// The amount of screen space the brush cursor occupies.
constexpr f32 BRUSH_CURSOR_SCREEN_PERCENTAGE = 0.005f;

// The amount of screen space the normals of the wall occupy.
constexpr f32 WALL_NORMAL_SCREEN_PERCENTAGE = 0.01f;

constexpr color4 BRUSH_WALL_COL1           = colors::WHITE;
constexpr color4 BRUSH_WALL_COL2           = colors::WHITE;
constexpr f32    BRUSH_WALL_FLASH_INTERVAL = 0.5f;

}

namespace nc
{

//==================================================================================================
// World coords to screen coords.
static mat3 calc_view_matrix_impl(vec2 offset, f32 zoom, f32 aspect)
{
  vec3 zooming = vec3{zoom, zoom * aspect, 1.0f};
  vec3 c0 = vec3{1.0f, 0.0f, 0.0f} * zooming;
  vec3 c1 = vec3{0.0f, 1.0f, 0.0f} * zooming;
  vec3 c2 = vec3{-offset,    1.0f} * zooming;
  return mat3{c0, c1, c2};
}

//==================================================================================================
struct EditorPrimitive : public std::enable_shared_from_this<EditorPrimitive>
{
  MeshHandle handle     = MeshHandle::invalid();
  aabb2      bbox       = aabb2{};
  f32        line_width = 1.0f;
  color4     color      = colors::WHITE;

  EditorPrimitive() = default;

  // Disable any moving or construction whatsoever
  EditorPrimitive(const EditorPrimitive&)            = delete;
  EditorPrimitive& operator=(const EditorPrimitive&) = delete;
  EditorPrimitive(EditorPrimitive&&)                 = delete;
  EditorPrimitive& operator=(EditorPrimitive&&)      = delete;

  void refresh_gpu_data(std::span<vec2> points)
  {
    // Get rid of the old data
    MeshManager::get().destroy_editor_primitive(handle);

    // Refresh the bbox if any vertices
    if (points.size())
    {
      bbox = aabb2{};
      for (vec2 p : points)
      {
        bbox.insert_point(p);
      }

      // Upload the data
      handle = MeshManager::get().create_editor_primitive(points, GL_LINES);
    }
  }

  ~EditorPrimitive()
  {
    // It should be ok to call this even if the handle is null
    MeshManager::get().destroy_editor_primitive(handle);
  }
};

using EditorPrimitivePtr = std::shared_ptr<EditorPrimitive>;
using RenderList         = std::vector<EditorPrimitivePtr>;

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
class EditorRenderer
{
public:
  EditorRenderer()
  : grid_rendering(ShaderProgramHandle::from_files(shaders::editor::lines::VERTEX_FILE, shaders::editor::lines::FRAGMENT_FILE))
  {
    nc_assert(grid_rendering.is_valid());
  }

  void render(mat3 projection, const std::span<EditorPrimitivePtr> primitives)
  {
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Editor pass");
    OpenGlStateScope state_restore;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    grid_rendering.use();
    grid_rendering.set_uniform(shaders::editor::lines::TRANSFORM, projection);

    for (const EditorPrimitivePtr primitive : primitives)
    {
      nc_assert(primitive->handle.is_valid());
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
  ivec2 pt;
};

//==================================================================================================
struct EditorSector
{
  EditorPrimitivePtr      render_data;
  std::vector<EditorWall> walls;
  f32                     floor_height = 0.0f;
  f32                     ceil_height  = 0.0f;

  void get_render_data(RenderList& list)
  {
    if (render_data && render_data->handle.is_valid())
    {
      list.push_back(render_data->shared_from_this());
    }
  }

  void recompute_render_data()
  {
    if (!render_data)
    {
      render_data = std::make_shared<EditorPrimitive>();
    }

    std::vector<vec2> points;
    for (u64 i = 1; i < walls.size(); ++i)
    {
      points.insert(points.end(), {cast<vec2>(walls[i-1].pt), cast<vec2>(walls[i].pt)});
    }

    render_data->refresh_gpu_data(points);
  }
};

//==================================================================================================
namespace ImGuiNc
{

bool ImageButton(cstr texture, ImVec2 size, bool selected)
{
  auto& texture_man = TextureManager::get();

  const auto& texture_handle   = texture_man[texture];
  const auto& texture_bundle   = texture_man.get_atlas_bundle(texture_handle.get_lifetime());
  ImTextureID bundle_im_handle = recast<ImTextureID>(cast<u64>(texture_bundle.diffuse_handle));

  vec2 bundle_size = texture_bundle.get_size();
  vec2 uv0 =  texture_handle.get_pos()                              / bundle_size;
  vec2 uv1 = (texture_handle.get_pos() + texture_handle.get_size()) / bundle_size;

  constexpr ImVec4 back_col     = ImVec4{0.0f, 0.0f, 0.0f, 0.0f};
  constexpr ImVec4 selected_col = ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
  constexpr ImVec4 default_col  = ImVec4{0.3f, 0.3f, 0.3f, 1.0f};

  ImVec4 tint_col = selected ? selected_col : default_col;

  return ImGui::ImageButton
  (
    texture, bundle_im_handle, size, ImVec2{uv0.x, uv0.y}, ImVec2{uv1.x, uv1.y}, back_col, tint_col
  );
}

}

//==================================================================================================
struct Editor::EditorImpl
{
  static constexpr u64 PX_PER_M  = 48;
  static constexpr u64 NUM_GRIDS = 5;

  static constexpr u64    GRID_SIZES[]  = {1, PX_PER_M, PX_PER_M * 8, PX_PER_M * 64, PX_PER_M * 512};
  static constexpr color4 GRID_COLORS[] =
  {
    colors::GRAY, colors::WHITE, colors::RED, colors::BLUE, colors::MAGENTA
  };

  static_assert(ARRAY_LENGTH(GRID_SIZES)  == NUM_GRIDS);
  static_assert(ARRAY_LENGTH(GRID_COLORS) == NUM_GRIDS);

  f32                             aspect = 1.0f;
  ivec2                           screen_size = ivec2{1920, 1080};
  std::vector<EditorPrimitivePtr> grids;
  std::vector<EditorSector>       sectors;
  EditorRenderer                  renderer;
  vec2                            center = VEC2_ZERO;
  f32                             zoom   = 0.0f;
  u64                             current_snap = 1;
  f64                             time_since_start = 0.0f;

  void update(f32 dt)
  {
    this->time_since_start += cast<f64>(dt);

    constexpr ImVec2 TOOL_SIZE = ImVec2{16, 16};

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

      bool has_select_tool = this->has_tool_selected<EmptyTool>();
      if (ImGuiNc::ImageButton("editor_select_tool", TOOL_SIZE, has_select_tool))
      {
        //this->change_tool<>
      }

      bool has_brush_tool = this->has_tool_selected<BrushTool>();
      if (ImGuiNc::ImageButton("editor_brush_tool", TOOL_SIZE, has_brush_tool))
      {
        this->change_tool<BrushTool>();
      }

      bool has_entity_tool = this->has_tool_selected<EmptyTool>();
      if (ImGuiNc::ImageButton("editor_entity_tool", TOOL_SIZE, has_entity_tool))
      {
        
      }

      ImGui::EndMainMenuBar();
    }

    this->handle_dragging() || this->handle_zoom_in_out();

    std::visit([&](auto& tool)
    {
      tool.update(*this, dt);
    }, this->tool);
  }

  bool try_insert_sector_walls_into_map(const std::vector<ivec2>& pts)
  {
    // First, check if the first point matched with the last one
    if (pts.size() < 3)
    {
      return false;
    }

    // Full loop
    if (pts.front() == pts.back())
    {
      EditorSector& new_sector = sectors.emplace_back();
      std::transform(pts.begin(), pts.end(), std::back_inserter(new_sector.walls), [&](ivec2 point)
      {
        return EditorWall{.pt = point};
      });

      new_sector.recompute_render_data();

      return true;
    }

    return false;
  }

  struct EmptyTool
  {
    void get_render_data(RenderList&) {}
    void update(EditorImpl&, f32)     {}
  };

  struct BrushTool
  {
    EditorPrimitivePtr cursor      = std::make_shared<EditorPrimitive>();
    EditorPrimitivePtr render_data = std::make_shared<EditorPrimitive>();
    std::vector<ivec2> painted_walls_stack;

    void update(EditorImpl& editor, f32 /*delta*/)
    {
      bool input_allowed = !ImGui::GetIO().WantCaptureMouse;
      bool left_click    = input_allowed && ImGui::IsMouseReleased(ImGuiMouseButton_Left);
      bool right_click   = input_allowed && ImGui::IsMouseReleased(ImGuiMouseButton_Right);

      vec2 mouse_world_pos = editor.get_mouse_wpos();
      editor.snap_to_grid(mouse_world_pos);

      mat3 screen_to_world = inverse(editor.calc_view_matrix());
      f32  normal_len      = screen_to_world[0].x * editor::WALL_NORMAL_SCREEN_PERCENTAGE;
      f32  cursor_len      = screen_to_world[0].x * editor::BRUSH_CURSOR_SCREEN_PERCENTAGE;

      // Cursor
      vec2 cursor_dirs[4] = {VEC2_X, VEC2_Y, -VEC2_X, -VEC2_Y};
      std::vector<vec2> cursor_pts;
      for (u64 i = 0; i < 4; ++i)
      {
        vec2 a = mouse_world_pos + cursor_len * cursor_dirs[i];
        vec2 b = mouse_world_pos + cursor_len * cursor_dirs[(i+1) % 4];
        cursor_pts.insert(cursor_pts.end(), {a, b});
      }
      cursor->refresh_gpu_data(cursor_pts);

      // Wall pts
      std::vector<vec2> pts_to_render;
      auto& wall_pts = this->painted_walls_stack;

      if (left_click)
      {
        ivec2 coords = ivec2{mouse_world_pos};
        if (wall_pts.empty() || wall_pts.back() != coords)
        {
          // Check if this will produce some sector
          // First, check our points
          wall_pts.push_back(coords);
          if (editor.try_insert_sector_walls_into_map(wall_pts))
          {
            // Sector created
            wall_pts.clear();
          }
        }
      }
      else if (right_click)
      {
        if (wall_pts.size())
        {
          wall_pts.pop_back();
        }
      }

      for (u64 i = 1; i < wall_pts.size(); ++i)
      {
        pts_to_render.insert(pts_to_render.end(), {vec2{wall_pts[i-1]}, vec2{wall_pts[i]}});
      }

      if (wall_pts.size())
      {
        pts_to_render.insert(pts_to_render.end(), {vec2{wall_pts.back()}, mouse_world_pos});
      }

      u64 pts_size = pts_to_render.size();
      nc_assert(pts_size % 2 == 0);

      for (u64 i = 0; i < pts_size; i += 2)
      {
        vec2 a = vec2{pts_to_render[i  ]};
        vec2 b = vec2{pts_to_render[i+1]};
        vec2 mid  = (a + b) * 0.5f;
        vec2 dir  = normalize_or_zero(b - a);
        vec2 norm = flipped(dir) * normal_len;
        pts_to_render.insert(pts_to_render.end(), {mid, mid + norm});
      }

      f32 color_mix = cast<f32>(abs(sin(editor.time_since_start / editor::BRUSH_WALL_FLASH_INTERVAL)));

      this->render_data->refresh_gpu_data(pts_to_render);
      this->render_data->color = mix(editor::BRUSH_WALL_COL1, editor::BRUSH_WALL_COL2, color_mix);
    }

    void get_render_data(RenderList& list)
    {
      if (render_data->handle.is_valid())
      {
        list.push_back(render_data->shared_from_this());
      }

      if (cursor->handle.is_valid())
      {
        list.push_back(cursor->shared_from_this());
      }
    }
  };

  std::variant<EmptyTool, BrushTool> tool;

  bool is_dragging                      = false;
  vec2 dragging_start_cursor_screen_pos = VEC2_ZERO;
  vec2 dragging_start_center_world_pos  = VEC2_ZERO;

  template<typename ToolType>
  bool has_tool_selected()
  {
    return std::holds_alternative<ToolType>(this->tool);
  }

  template<typename NewToolType>
  void change_tool()
  {
    if (this->has_tool_selected<NewToolType>())
    {
      return;
    }

    // Create the data
    this->tool = NewToolType{};
  }

  void snap_to_grid(vec2& coords)
  {
    coords = round(coords);
  }

  vec2 get_offset() const
  {
    return this->center;
  }

  void set_offset(vec2 new_offset)
  {
    if (vec2 prev = this->center; prev != new_offset)
    {
      this->center = new_offset;
      this->on_offset_changed(prev, this->center);
    }
  }

  void on_offset_changed(vec2 /*prev*/, vec2 /*curr*/)
  {
    this->recompute_grids();
  }

  f32 get_zoom() const
  {
    return this->zoom;
  }

  void set_zoom(f32 new_zoom)
  {
    if (f32 prev = this->zoom; prev != new_zoom)
    {
      this->zoom = new_zoom;
      this->on_zoom_changed(prev, this->zoom);
    }
  }

  void on_zoom_changed(f32 /*previous_zoom*/, f32 /*new_zoom*/)
  {
    this->recompute_grids();
  }

  void recompute_exact_grid(EditorPrimitive& primitive, u64 step_size, color4 color)
  {
    nc_assert(step_size > 0);

    // compute left/right/up/down world positions
    vec2 bottom_left = this->screen_to_wpos(vec2{-1.0f, -1.0f});
    vec2 top_right   = this->screen_to_wpos(vec2{ 1.0f,  1.0f});

    f32  world_size   = cast<f32>(step_size);
    vec2 screen_scale = (this->calc_view_matrix() * vec3{world_size, world_size, 0.0f}).xy;
    f32  percentage_of_screen = compMax(screen_scale) * 0.5f;

    std::vector<vec2> points;

    f32 alpha_coeff = 1.0f;
    f32 thresh = editor::GRID_SCREEN_PERCENTAGE_FOR_VISIBILITY;

    if (percentage_of_screen > thresh)
    {
      alpha_coeff = clamp((percentage_of_screen - thresh) / thresh, 0.0f, 1.0f);

      s64 x_start = cast<s64>(ceil(bottom_left.x) / step_size) * step_size;
      s64 x_end   = cast<s64>(top_right.x         / step_size) * step_size;
      s64 y_start = cast<s64>(ceil(bottom_left.y) / step_size) * step_size;
      s64 y_end   = cast<s64>(top_right.y         / step_size) * step_size;

      for (s64 x = x_start; x <= x_end; x += step_size)
      {
        vec2 from = vec2{cast<f32>(x), top_right.y};
        vec2 to   = vec2{cast<f32>(x), bottom_left.y};
        points.insert(points.end(), {from, to});
      }

      for (s64 y = y_start; y <= y_end; y += step_size)
      {
        vec2 from = vec2{bottom_left.x, cast<f32>(y)};
        vec2 to   = vec2{top_right.x,   cast<f32>(y)};
        points.insert(points.end(), {from, to});
      }
    }

    primitive.refresh_gpu_data(points);
    primitive.line_width = 1.0f;
    primitive.color      = color4{color.xyz, editor::GRID_ALPHA * alpha_coeff};
  }

  void recompute_grids()
  {
    // grid sizes with different colors?
    grids.resize(NUM_GRIDS);
    for (u64 i = 0; i < NUM_GRIDS; ++i)
    {
      grids[i] = std::make_shared<EditorPrimitive>();
      this->recompute_exact_grid(*this->grids[i], GRID_SIZES[i], GRID_COLORS[i]);
    }
  }

  void init()
  {
    this->recompute_grids();
    this->change_tool<BrushTool>();
  }

  mat3 calc_view_matrix()
  {
    return calc_view_matrix_impl(this->get_offset(), std::exp(this->get_zoom() * 0.1f), this->aspect);
  }

  vec2 screen_to_wpos(vec2 screen_pos)
  {
    mat3 screen_to_world = inverse(this->calc_view_matrix());
    return (screen_to_world * vec3{screen_pos, 1.0f}).xy;
  }

  vec2 wpos_to_screen(vec2 wpos)
  {
    mat3 world_to_screen = this->calc_view_matrix();
    return (world_to_screen * vec3{wpos, 1.0f}).xy;
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
    return this->screen_to_wpos(this->get_mouse_screen_pos());
  }

  bool handle_dragging()
  {
    bool middle_mouse_held = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

    if (middle_mouse_held != is_dragging)
    {
      // Dragging state changed
      is_dragging = middle_mouse_held;
      if (is_dragging)
      {
        // Started dragging
        this->dragging_start_center_world_pos  = this->get_offset();
        this->dragging_start_cursor_screen_pos = this->get_mouse_screen_pos();
      }
    }

    // During the dragging
    if (is_dragging)
    {
      vec2 screen_diff_from_start = this->get_mouse_screen_pos() - this->dragging_start_cursor_screen_pos;
      mat3 screen_to_world = inverse(this->calc_view_matrix());
      vec2 wspace_diff_from_start = (screen_to_world * vec3{screen_diff_from_start, 0.0f}).xy;
      this->set_offset(this->dragging_start_center_world_pos - wspace_diff_from_start);
    }

    ImGui::SetMouseCursor(is_dragging ? ImGuiMouseCursor_ResizeAll : ImGuiMouseCursor_Arrow);
    return is_dragging;
  }

  bool handle_zoom_in_out()
  {
    f32 wheel = ImGui::GetIO().MouseWheel;
    wheel = floor(wheel * 10.0f) * 0.1f;

    if (!wheel)
    {
      return false;
    }

    vec2 world_pos_under_cursor_before_zoom = this->get_mouse_wpos();
    f32 new_zoom = clamp(this->get_zoom() + wheel, -100.0f, 100.0f);
    this->set_zoom(new_zoom);
    vec2 world_pos_under_cursor_after_zoom = this->get_mouse_wpos();
    vec2 difference = world_pos_under_cursor_after_zoom - world_pos_under_cursor_before_zoom;
    this->set_offset(this->get_offset() - difference);

    return true;
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
void Editor::update(f32 delta)
{
  m_impl->update(delta);
}

//==================================================================================================
void Editor::render()
{
  RenderList primitives;

  for (u64 i = 0; i < EditorImpl::NUM_GRIDS; ++i)
  {
    if (m_impl->grids[i]->handle.is_valid())
    {
      primitives.push_back(m_impl->grids[i]->shared_from_this());
    }
  }

  std::visit([&](auto& data_type)
  {
    data_type.get_render_data(primitives);
  },
  m_impl->tool);

  for (EditorSector& sector : m_impl->sectors)
  {
    sector.get_render_data(primitives);
  }

  mat3 projection = m_impl->calc_view_matrix();
  m_impl->renderer.render(projection, primitives);
}

//==================================================================================================
void Editor::on_window_resized(u32 width, u32 height)
{
  m_impl->aspect      = cast<f32>(width) / height;
  m_impl->screen_size = ivec2{width, height};
}

}

#endif // #if NC_EDITOR
