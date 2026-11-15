// Project Nuclidean Source File

#include <config.h>

#if NC_EDITOR

#include <editor/editor_brush_tool.h>
#include <editor/editor_impl.h>

#include <editor/editor_primitive.h>
#include <editor/editor_level.h>

#include <math/lingebra.h>
#include <math/utils.h>
#include <intersect.h> // dist::point_line_2d

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
  bool shift         = ImGui::GetIO().KeyShift;
  bool ctrl          = ImGui::GetIO().KeyCtrl;

  vec2 cursor_world_pos = editor.get_mouse_wpos();
  vec2 mouse_world_pos  = editor.snap_to_grid_inplace(cursor_world_pos);

  EditorCoord pointing_at = EditorCoord{mouse_world_pos};

  // Preview lines and their color
  std::vector<vec2> points;
  color4 line_color   = colors::WHITE;
  color4 cursor_color = is_within_level_bounds(pointing_at) ? colors::WHITE : colors::RED;

  // Splitting lines (shift) and splicing points (ctrl) is possible only while not painting
  bool splitting_line = !is_painting && shift;
  bool splicing_point = !is_painting && ctrl && !shift;

  // When splitting or splicing, the cursor is shown only if there is a line or a point to do it with
  bool show_cursor = !splitting_line && !splicing_point;

  if (splitting_line)
  {
    // Find the line closest to the cursor
    EditorID closest_line      = INVALID_EDITOR_ID;
    f32      closest_line_dist = FLT_MAX;

    for (const auto&[id, object] : editor.level.objects)
    {
      if (!std::holds_alternative<EditorLine>(object))
      {
        continue;
      }

      vec2 a = cast<vec2>(editor.level.get_line_coord(id, true));
      vec2 b = cast<vec2>(editor.level.get_line_coord(id, false));

      f32 dist = dist::point_line_2d(cursor_world_pos, a, b);
      if (dist < closest_line_dist)
      {
        closest_line_dist = dist;
        closest_line      = id;
      }
    }

    // The distance is in the world space, the threshold is in the screen space
    if (closest_line != INVALID_EDITOR_ID && editor.get_zoom_factor() * closest_line_dist < editor::BRUSH_POINTING_THRESHOLD)
    {
      // Preview the two new lines, the new point is where the cursor is
      bool can_split = editor.level.can_split_line(closest_line, pointing_at);
      line_color     = can_split ? colors::WHITE : colors::RED;
      cursor_color   = line_color;
      show_cursor    = true;

      vec2 a = cast<vec2>(editor.level.get_line_coord(closest_line, true));
      vec2 b = cast<vec2>(editor.level.get_line_coord(closest_line, false));
      points.insert(points.end(), {a, cast<vec2>(pointing_at), cast<vec2>(pointing_at), b});

      if (left_click && can_split)
      {
        ActionSplitOrSpliceLine action;
        action.init_split(editor.level, closest_line, pointing_at);
        editor.do_action(std::move(action));
      }
    }
  }
  else if (splicing_point)
  {
    // Find the point closest to the cursor
    EditorID closest_point      = INVALID_EDITOR_ID;
    f32      closest_point_dist = FLT_MAX;

    for (const auto&[id, object] : editor.level.objects)
    {
      const EditorPoint* point = std::get_if<EditorPoint>(&object);
      if (!point)
      {
        continue;
      }

      f32 dist = distance(cursor_world_pos, cast<vec2>(point->coords));
      if (dist < closest_point_dist)
      {
        closest_point_dist = dist;
        closest_point      = id;
      }
    }

    // The distance is in the world space, the threshold is in the screen space
    if (closest_point != INVALID_EDITOR_ID && editor.get_zoom_factor() * closest_point_dist < editor::BRUSH_POINTING_THRESHOLD)
    {
      bool can_splice = editor.level.can_splice_point(closest_point);
      line_color      = can_splice ? colors::WHITE : colors::RED;
      cursor_color    = line_color;
      show_cursor     = true;

      // The cursor sticks to the point that gets replaced
      pointing_at = editor.level.get_object<EditorPoint>(closest_point).coords;

      // Preview the line between both neighbors, if there are exactly two of them
      const std::vector<EditorID>& half_edges = editor.level.point_to_half_edges.at(closest_point);
      if (half_edges.size() == 2)
      {
        EditorID twin_1 = editor.level.get_object<EditorHalfEdge>(half_edges[0]).twin;
        EditorID twin_2 = editor.level.get_object<EditorHalfEdge>(half_edges[1]).twin;
        vec2     a      = cast<vec2>(editor.level.get_object<EditorPoint>(editor.level.get_object<EditorHalfEdge>(twin_1).from).coords);
        vec2     b      = cast<vec2>(editor.level.get_object<EditorPoint>(editor.level.get_object<EditorHalfEdge>(twin_2).from).coords);
        points.insert(points.end(), {a, b});
      }

      if (left_click && can_splice)
      {
        ActionSplitOrSpliceLine action;
        action.init_splice(editor.level, closest_point);
        editor.do_action(std::move(action));
      }
    }
  }
  else
  {
    bool should_reset   = false;
    bool should_start   = false;
    bool sector_created = false;

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

  if (show_cursor)
  {
    this->points_render_data.push_back(EditorPrimitiveAndProperties{editor.get_disc_geometry(), props});
  }

  if (is_painting)
  {
    props.transform[2] = vec3{painting_start, 1.0f};
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
