// Project Nuclidean Source File

#include <config.h>

#if NC_EDITOR

#include <editor/editor_brush_tool.h>
#include <editor/editor_impl.h>

#include <editor/editor_primitive.h>
#include <editor/editor_level.h>

#include <math/lingebra.h>
#include <math/utils.h>

#include <imgui/imgui.h>

#include <vector>

namespace nc
{

//==================================================================================================
void BrushTool::update(f32 /*delta*/)
{
  bool input_allowed = !ImGui::GetIO().WantCaptureMouse;
  bool left_click    = input_allowed && ImGui::IsMouseReleased(ImGuiMouseButton_Left);
  bool right_click   = input_allowed && ImGui::IsMouseReleased(ImGuiMouseButton_Right);

  vec2 mouse_world_pos = editor.get_mouse_wpos();
  editor.snap_to_grid(mouse_world_pos);

  mat3 screen_to_world = inverse(editor.calc_view_matrix());
  f32  cursor_len      = screen_to_world[0].x * editor::BRUSH_CURSOR_SCREEN_PERCENTAGE;

  // Cursor
  constexpr vec2 CURSOR_DIRS[4] = {VEC2_X, VEC2_Y, -VEC2_X, -VEC2_Y};
  std::vector<vec2> cursor_pts;
  for (u64 i = 0; i < 4; ++i)
  {
    vec2 a = mouse_world_pos + cursor_len * CURSOR_DIRS[i];
    vec2 b = mouse_world_pos + cursor_len * CURSOR_DIRS[(i+1) % 4];
    cursor_pts.insert(cursor_pts.end(), {a, b});
  }
  cursor->refresh_gpu_data(cursor_pts);

  EditorCoord pointing_at    = EditorCoord{mouse_world_pos};
  bool        should_reset   = false;
  bool        should_start   = false;
  bool        sector_created = false;

  // Check what to do
  if (left_click && is_painting && editor.try_insert_line_into_map(painting_start, pointing_at, sector_created))
  {
    // We inserted a line into the map, reset!
    if (sector_created)
    {
      should_reset = true;
    }
    else
    {
      should_start = true;
    }
  }
  else if (left_click && !is_painting)
  {
    // We started painting
    should_start = true;
  }
  else if (right_click && is_painting)
  {
    // We canceled the first point, reset
    should_reset = true;
  }

  // And then do it
  if (should_reset)
  {
    is_painting    = false;
    painting_start = EditorCoord{0};
  }
  else if (should_start)
  {
    is_painting    = true;
    painting_start = pointing_at;
  }

  // Wall pts
  std::vector<vec2> points;
  color4 line_color = colors::WHITE;
  if (is_painting)
  {
    f32 color_mix = cast<f32>(abs(sin(editor.time_since_start / editor::BRUSH_WALL_FLASH_INTERVAL)));
    line_color = mix(editor::BRUSH_WALL_COL1, editor::BRUSH_WALL_COL2, color_mix);

    // Set a red color if we can't put a line here!
    if (!editor.level.can_create_line(painting_start, pointing_at))
    {
      line_color = colors::RED;
    }

    // Create the line from the 2 points
    points.insert(points.end(), {cast<vec2>(painting_start), cast<vec2>(pointing_at)});
  }

  // And then refresh the GPU data!
  this->render_data->refresh_gpu_data(points);
  this->render_data->properties.color = line_color;
  this->render_data->properties.order = 99;
}

//==================================================================================================
void BrushTool::get_render_data(RenderList& list)
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

//==================================================================================================
void BrushTool::get_modifiers(RenderModifierList& /*list*/)
{

}

}

#endif // #if NC_EDITOR
