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
  color4 line_color   = colors::WHITE;
  color4 cursor_color = is_within_level_bounds(pointing_at) ? colors::WHITE : colors::RED;
  if (is_painting)
  {
    f32 color_mix = cast<f32>(abs(sin(editor.time_since_start / editor::BRUSH_WALL_FLASH_INTERVAL)));
    line_color = mix(editor::BRUSH_WALL_COL1, editor::BRUSH_WALL_COL2, color_mix);

    // Set a red color if we can't put a line here! The cursor is the end of the line, so as well.
    if (!editor.level.can_create_line(painting_start, pointing_at))
    {
      line_color   = colors::RED;
      cursor_color = colors::RED;
    }

    // Create the line from the 2 points
    points.insert(points.end(), {cast<vec2>(painting_start), cast<vec2>(pointing_at)});
  }

  // And then refresh the GPU data!
  this->render_data->refresh_gpu_data(points);
  this->render_data->properties.color = line_color;
  this->render_data->properties.order = 99;

  this->points_render_data.clear();

  EditorPrimitiveRenderingProperties props = EditorPrimitiveRenderingProperties::calc_point_override_props
  (
    cast<vec2>(pointing_at), editor::POINT_SIZE, editor.get_zoom_factor(), INVALID_EDITOR_ID
  );

  props.color          = cursor_color;
  props.override_color = true;
  props.order          = 100;
  props.override_order = true;

  this->points_render_data.push_back(EditorPrimitiveAndProperties{editor.get_disc_geometry(), props});

  if (is_painting)
  {
    props.transform[3] = vec3{painting_start, 1.0f};
    props.order        = 99;
    props.color        = line_color;
    this->points_render_data.push_back(EditorPrimitiveAndProperties{editor.get_disc_geometry(), props});
  }
}

//==================================================================================================
void BrushTool::get_render_data(RenderList& list)
{
  if (render_data->handle.is_valid())
  {
    list.push_back(render_data->shared_from_this());
  }

  for (const auto&[primitive, props] : this->points_render_data)
  {
    list.push_back(primitive, props);
  }
}

//==================================================================================================
void BrushTool::get_modifiers(RenderModifierList& /*list*/)
{

}

}

#endif // #if NC_EDITOR
