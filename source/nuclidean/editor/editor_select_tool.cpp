// Project Nuclidean Source File

#include <config.h>

#if NC_EDITOR

#include <editor/editor_select_tool.h>
#include <editor/editor_impl.h>
#include <common.h> // nc_assert

#include <editor/editor_primitive.h>
#include <editor/editor_level.h>
#include <editor/editor_objects.h>
#include <editor/editor_objects_render.h>
#include <editor/editor_actions.h>

#include <math/lingebra.h>
#include <math/utils.h>
#include <intersect.h>

#include <imgui/imgui.h>

#include <stack_vector.h>

#include <vector>
#include <variant>
#include <memory>
#include <map>
#include <set>
#include <algorithm> // std::ranges::find

namespace nc
{

//==================================================================================================
void SelectTool::modify_rendering_properties
(
  EditorPrimitiveRenderingProperties& properties,
  const EditorPrimitive&              /*primitive*/
)
{
  EditorID author = properties.author_id;

  // First color the selection in green..
  if (std::ranges::find(this->selected_objects, author) != this->selected_objects.end())
  {
    // It is selected!
    properties.color = color4{0.2f, 1.0f, 0.3f, properties.color.a};
    properties.line_width *= 2.0f;
  }

  // Then color the pointed-at object
  if (author == this->pointed_at_object && author != INVALID_EDITOR_ID)
  {
    properties.color = color4{1.0f, 1.0f, 0.0f, properties.color.a};
    properties.line_width *= 4.0f;

    // Increase the scale of points when pointed at
    bool is_point = editor.level.try_get_object<EditorPoint>(author) != nullptr;
    if (is_point)
    {
      properties.transform[0] *= 2.5f;
      properties.transform[1] *= 2.5f;
    }
  }
}

//==================================================================================================
void SelectTool::get_render_data(RenderList& list)
{
  if (this->dragging_outline_lines->is_valid())
  {
    list.push_back(this->dragging_outline_lines);
  }

  for (const auto&[primitive, props] : this->dragging_outline_points)
  {
    list.push_back(primitive, props);
  }
}

//==================================================================================================
static void collect_points_of_object
(
  const EditorLevel& level, EditorID object_id, std::set<EditorID>& points_out
)
{
  const EditorObject* object_ptr = level.get_any_object(object_id);
  if (!object_ptr)
  {
    return;
  }

  if (const EditorSector* sector = std::get_if<EditorSector>(object_ptr))
  {
    EditorID half_edge_id = sector->edge;
    do
    {
      const EditorHalfEdge& hedge = level.get_object<EditorHalfEdge>(half_edge_id);
      points_out.insert(hedge.from);
      half_edge_id = hedge.next;
    }
    while (half_edge_id != sector->edge);
  }
  else if (const EditorLine* line = std::get_if<EditorLine>(object_ptr))
  {
    points_out.insert(level.get_object<EditorHalfEdge>(line->half_edge_a).from);
    points_out.insert(level.get_object<EditorHalfEdge>(line->half_edge_b).from);
  }
  else if (const EditorPoint* point = std::get_if<EditorPoint>(object_ptr))
  {
    points_out.insert(object_id);
  }
  else
  {
    nc_assert(false, "Not implemented?!");
  }
}

//==================================================================================================
static void calc_move_point_geometry
(
  EditorImpl&                                editor,
  const std::map<EditorID, EditorCoord>&     moved_points,
  EditorPrimitive&                           primitive_lines_out,
  std::vector<EditorPrimitiveAndProperties>& primitive_points_out
)
{
  std::vector<vec2>  geometry_lines;
  std::vector<vec2>  geometry_points;
  std::set<EditorID> lines;

  auto get_point_coord = [&](EditorID point_id)->EditorCoord
  {
    nc_assert(editor.level.get_any_object(point_id));
    auto it = moved_points.find(point_id);
    if (it == moved_points.end())
    {
      return editor.level.get_object<EditorPoint>(point_id).coords;
    }

    return it->second;
  };

  // Collect all lines that neighbor the points
  for (const auto&[point_id, _] : moved_points)
  {
    nc_assert(editor.level.get_any_object(point_id), "Must exist!");
    nc_assert(editor.level.point_to_half_edges.contains(point_id));

    for (EditorID half_edge_id : editor.level.point_to_half_edges[point_id])
    {
      const EditorHalfEdge& hedge = editor.level.get_object<EditorHalfEdge>(half_edge_id);
      nc_assert(editor.level.get_any_object(hedge.line) != nullptr);
      lines.insert(hedge.line);
    }
  }

  // Build some geometry from the lines
  for (EditorID line_id : lines)
  {
    const EditorLine&     line = editor.level.get_object<EditorLine>(line_id);
    const EditorHalfEdge& h1   = editor.level.get_object<EditorHalfEdge>(line.half_edge_a);
    const EditorHalfEdge& h2   = editor.level.get_object<EditorHalfEdge>(line.half_edge_b);
    EditorCoord           c1   = get_point_coord(h1.from);
    EditorCoord           c2   = get_point_coord(h2.from);
    geometry_lines.insert(geometry_lines.end(), {c1, c2});
  }

  // Build some geometry from the points
  for (const auto&[point_id, moved_coord] : moved_points)
  {
    vec2 pt = cast<vec2>(moved_coord);

    EditorPrimitivePtr primitive = editor.get_disc_geometry();
    EditorPrimitiveRenderingProperties props = EditorPrimitiveRenderingProperties::calc_point_override_props
    (
      vec2{moved_coord}, 0.003f, editor.get_zoom_factor(), INVALID_EDITOR_ID
    );

    primitive_points_out.push_back({primitive, props});
  }

  // Refill the GPU buffers
  primitive_lines_out.refresh_gpu_data(geometry_lines);
}

//==================================================================================================
bool SelectTool::handle_dragging()
{
  // Reset these always
  this->dragging_outline_lines = std::make_shared<EditorPrimitive>();
  this->dragging_outline_points.clear();

  // At this point we know that the IDs belong to existing objects because we called the
  // "deselect_non_existing_objects" before this.
  bool is_pointing_at_object = this->pointed_at_object != INVALID_EDITOR_ID;
  bool is_something_selected = this->selected_objects.size();

  if (!is_pointing_at_object && !is_something_selected)
  {
    return false;
  }

  bool input_allowed  = !ImGui::GetIO().WantCaptureMouse;
  bool holding_right  = input_allowed && ImGui::IsMouseDown(ImGuiMouseButton_Right);
  bool released_mouse = false;

  if (holding_right != this->is_dragging)
  {
    this->is_dragging = holding_right;

    if (this->is_dragging)
    {
      // Started dragging.. Decide what do we actually want to drag
      // Compose a list of points to drag based on the current selection or pointed at object
      StackVector<EditorID, 16> objects_to_consider;

      if (is_something_selected)
      {
        objects_to_consider.assign(this->selected_objects.begin(), this->selected_objects.end());
      }
      else if (is_pointing_at_object)
      {
        objects_to_consider.push_back(this->pointed_at_object);
      }

      std::set<EditorID> points_to_drag;
      for (EditorID object : objects_to_consider)
      {
        collect_points_of_object(editor.level, object, points_to_drag);
      }

      // Set the dragged points
      this->dragged_points.assign(points_to_drag.begin(), points_to_drag.end());

      // Set the position of the mouse when we started dragging
      this->dragging_start_pt = editor.get_mouse_wpos();
    }
    else
    {
      // Ended dragging
      released_mouse = true;
    }
  }

  if (this->is_dragging || released_mouse)
  {
    vec2 cursor_in_world   = editor.get_mouse_wpos();
    vec2 offset_from_start = cursor_in_world - this->dragging_start_pt;
    EditorCoord offset_snapped = editor.snap_to_grid_inplace(offset_from_start);

    std::map<EditorID, EditorCoord> map_of_new_point_coords;

    for (EditorID pt_id : this->dragged_points)
    {
      const EditorPoint& point = editor.level.get_object<EditorPoint>(pt_id);

      map_of_new_point_coords[pt_id] = point.coords + offset_snapped;
    }

    calc_move_point_geometry
    (
      editor, map_of_new_point_coords, *this->dragging_outline_lines, this->dragging_outline_points
    );

    bool can_move_points = editor.level.can_move_points(map_of_new_point_coords);

    this->dragging_outline_lines->properties.color = can_move_points ? colors::WHITE : colors::RED;
    this->dragging_outline_lines->properties.order = 85;

    // Move the points if possible
    if (released_mouse)
    {
      if (can_move_points)
      {
        ActionMovePoints move_action;
        move_action.init_with_new_coords(editor.level, std::move(map_of_new_point_coords));
        editor.do_action(std::move(move_action));
      }

      // Reset the dragged points list
      this->dragged_points.clear();
    }
  }

  // Signal if we should handle the selection or not
  return this->is_dragging;
}

//==================================================================================================
bool SelectTool::handle_selection()
{
  vec2 cursor_pos_in_world = editor.get_mouse_wpos();

  EditorID sector_directly_selected, closest_wall, closest_point;
  sector_directly_selected = closest_wall = closest_point = INVALID_EDITOR_ID;

  f32 dist_to_closest_point = FLT_MAX;
  f32 dist_to_closest_wall  = FLT_MAX;

  // Update the pointed at sector..
  for (const auto&[id, object] : editor.objects_render)
  {
    // Handle the direct point-at sector - iterate all convex parts
    const EditorSectorRenderData* sector_ptr = std::get_if<EditorSectorRenderData>(&object);
    if (!sector_ptr)
    {
      continue;
    }

    const EditorSectorRenderData& sector = *sector_ptr;
    for (const auto& convex_part_indices : sector.convex_parts)
    {
      u16 idx0 = convex_part_indices[0];

      for (u64 i = 0; i < convex_part_indices.size(); ++i)
      {
        u16 idx1 = convex_part_indices[ i                                  ];
        u16 idx2 = convex_part_indices[(i + 1) % convex_part_indices.size()];

        vec2 pt0 = cast<vec2>(sector.surface_points[idx0]);
        vec2 pt1 = cast<vec2>(sector.surface_points[idx1]);
        vec2 pt2 = cast<vec2>(sector.surface_points[idx2]);

        if (intersect::point_triangle(cursor_pos_in_world, pt0, pt1, pt2))
        {
          sector_directly_selected = id;
          goto end_render_object_loop; // thank you C++ committee for not allowing me to break from 3 loops
        }
      }
    }
  }
  end_render_object_loop:

  // Now check the proximity to the lines
  for (const auto&[id, obj] : editor.level.objects)
  {
    const EditorLine* line = std::get_if<EditorLine>(&obj);
    if (!line)
    {
      continue;
    }

    const EditorHalfEdge& h1 = editor.level.get_object<EditorHalfEdge>(line->half_edge_a);
    const EditorHalfEdge& h2 = editor.level.get_object<EditorHalfEdge>(line->half_edge_b);
    const EditorPoint&    p1 = editor.level.get_object<EditorPoint>(h1.from);
    const EditorPoint&    p2 = editor.level.get_object<EditorPoint>(h2.from);

    f32 dist = dist::point_line_2d(cursor_pos_in_world, cast<vec2>(p1.coords), cast<vec2>(p2.coords));
    if (dist < dist_to_closest_wall)
    {
      dist_to_closest_wall = dist;
      closest_wall         = id;
    }
  }

  // And now the proximity to the points
  for (const auto&[id, obj] : editor.level.objects)
  {
    const EditorPoint* point = std::get_if<EditorPoint>(&obj);
    if (!point)
    {
      continue;
    }

    f32 dist = distance(cursor_pos_in_world, cast<vec2>(point->coords));
    if (dist < dist_to_closest_point)
    {
      dist_to_closest_point = dist;
      closest_point         = id;
    }
  }

  constexpr f32 SELECT_THRESHOLD = 0.01f;

  // The distance is in the world space, we need to convert it to the screen space
  f32 dist_to_closest_point_screen_space = editor.get_zoom_factor() * dist_to_closest_point;
  f32 dist_to_closest_wall_screen_space  = editor.get_zoom_factor() * dist_to_closest_wall;

  f32      weights[3]{};
  EditorID objects[3]{};

  weights[0] = dist_to_closest_point_screen_space * 0.1f;
  weights[1] = dist_to_closest_wall_screen_space;
  weights[2] = 1.0f;
  objects[0] = dist_to_closest_point_screen_space < SELECT_THRESHOLD ? closest_point : INVALID_EDITOR_ID;
  objects[1] = dist_to_closest_wall_screen_space  < SELECT_THRESHOLD ? closest_wall  : INVALID_EDITOR_ID;
  objects[2] = sector_directly_selected;

  s32 best_idx = -1;
  for (s32 i = 0; i < 3; ++i)
  {
    if ((objects[i] != INVALID_EDITOR_ID) && (best_idx < 0 || weights[i] < weights[best_idx]))
    {
      best_idx = i;
    }
  }

  this->pointed_at_object = best_idx >= 0 ? objects[best_idx] : INVALID_EDITOR_ID;

  // Handle adding/removing objects from selection list
  bool left_click = ImGui::IsKeyPressed(ImGuiKey_MouseLeft);
  if (left_click && this->pointed_at_object != INVALID_EDITOR_ID)
  {
    auto it = std::ranges::find(this->selected_objects, this->pointed_at_object);
    if (it == this->selected_objects.end())
    {
      // Not yet selected.. Add it
      this->selected_objects.push_back(this->pointed_at_object);
    }
    else
    {
      // It is selected, so remove it..
      this->selected_objects.erase(it);
    }
  }

  // Clear the selection on ESC
  bool esc_pressed = ImGui::IsKeyPressed(ImGuiKey_Escape);
  if (esc_pressed)
  {
    this->selected_objects.clear();
  }

  return true;
}

//==================================================================================================
void SelectTool::deselect_non_existing_objects()
{
  // Remove all non-existing objects from the selection
  std::erase_if(this->selected_objects, [&](EditorID id)
  {
    return editor.level.get_any_object(id) == nullptr;
  });

  // Reset the currently pointed id if it does not exist
  if (editor.level.get_any_object(this->pointed_at_object) == nullptr)
  {
    this->pointed_at_object = INVALID_EDITOR_ID;
  }
}

//==================================================================================================
void SelectTool::update(f32 /*dt*/)
{
  deselect_non_existing_objects();
  handle_dragging() || handle_selection();
}

//==================================================================================================
void SelectTool::get_modifiers(RenderModifierList& list)
{
  list.push_back(this);
}

}

#endif // #if NC_EDITOR
