// Project Nuclidean Source File

#include <config.h>

#if NC_EDITOR

#include <editor/editor_impl.h>
#include <common.h> // nc_assert

#include <editor/editor_primitive.h>
#include <editor/editor_renderer.h>
#include <editor/rendering_modifier.h>
#include <editor/editor_objects_render.h>
#include <editor/editor_objects.h>

#include <math/lingebra.h>   // compMax
#include <intersect.h>

#include <imgui/imgui.h>

#include <vector>
#include <variant>
#include <memory>
#include <map>
#include <set>

namespace nc
{

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
void EditorImpl::recompute_sector_render_data(const EditorSector& sector, EditorSectorRenderData& render_data)
{
  // Iterate all edges of a sector and push its points into the list..
  auto collect_walls = [&](const EditorSector& walled_sector, std::vector<EditorWall>& walls_out)
  {
    walls_out.clear();

    EditorID first_edge = walled_sector.edge;
    EditorID edge       = first_edge;

    do
    {
      const EditorHalfEdge& edge_ref = level.get_object<EditorHalfEdge>(edge);
      const EditorPoint&    pt_ref   = level.get_object<EditorPoint>(edge_ref.from);
      walls_out.push_back(EditorWall{.pt = pt_ref.coords});
      edge = edge_ref.next;
    }
    while (edge != first_edge);
  };

  collect_walls(sector, render_data.walls);

  // The holes are sectors of their own, we only need their outlines to cut them out of ours
  render_data.holes.clear();
  if (sector.first_hole != INVALID_EDITOR_ID)
  {
    EditorID hole_id = sector.first_hole;
    do
    {
      const EditorSector& hole = level.get_object<EditorSector>(hole_id);
      collect_walls(hole, render_data.holes.emplace_back());
      hole_id = hole.next_hole;
    }
    while (hole_id != sector.first_hole);
  }

  // Then recompute the render data
  render_data.recompute_render_data();
}

//==================================================================================================
void EditorImpl::on_object_created(EditorID object_id, const EditorSector& sector)
{
  if (object_id == VOID_SECTOR_ID)
  {
    // Void sector ignored
    return;
  }

  // Create a new sector and set its ID. Its render data gets computed together with all the
  // other dirty sectors at the end of the update.
  objects_render.insert({object_id, EditorSectorRenderData{.id = object_id}});

  // We might have become a hole of our parent, which has to cut us out of its surface
  dirty_sectors.insert(object_id);
  dirty_sectors.insert(sector.parent);
}

//==================================================================================================
void EditorImpl::on_object_created(EditorID object_id, const EditorLine& line)
{
  const EditorHalfEdge& h1 = level.get_object<EditorHalfEdge>(line.half_edge_a);
  const EditorHalfEdge& h2 = level.get_object<EditorHalfEdge>(line.half_edge_b);
  const EditorPoint&    p1 = level.get_object<EditorPoint>(h1.from);
  const EditorPoint&    p2 = level.get_object<EditorPoint>(h2.from);

  EditorLineRenderData render_data {.id = object_id};
  render_data.recreate_render_data(p1.coords, p2.coords);
  objects_render.insert({object_id, std::move(render_data)});
}

//==================================================================================================
void EditorImpl::on_object_destroyed(EditorID object_id, const EditorLine&)
{
  objects_render.erase(object_id);
}

//==================================================================================================
void EditorImpl::on_object_modified(EditorID object_id, const EditorLine&, const EditorLine& line)
{
  const EditorHalfEdge& h1 = level.get_object<EditorHalfEdge>(line.half_edge_a);
  const EditorHalfEdge& h2 = level.get_object<EditorHalfEdge>(line.half_edge_b);
  const EditorPoint&    p1 = level.get_object<EditorPoint>(h1.from);
  const EditorPoint&    p2 = level.get_object<EditorPoint>(h2.from);

  std::get<EditorLineRenderData>(objects_render[object_id]).recreate_render_data(p1.coords, p2.coords);
}

//==================================================================================================
void EditorImpl::on_object_destroyed(EditorID object_id, const EditorSector& sector)
{
  if (object_id == VOID_SECTOR_ID)
  {
    return;
  }

  // Destroy the sector..
  nc_assert(objects_render.contains(object_id));
  objects_render.erase(object_id);

  // If we were a hole then our parent does not have to cut us out anymore
  dirty_sectors.insert(sector.parent);
}

//==================================================================================================
void EditorImpl::on_object_modified(EditorID object_id, const EditorSector& old_state, const EditorSector& new_state)
{
  if (object_id == VOID_SECTOR_ID)
  {
    return;
  }

  nc_assert(objects_render.contains(object_id));

  // Our shape might have changed, and so might have the parent we are a hole of
  dirty_sectors.insert(object_id);
  dirty_sectors.insert(old_state.parent);
  dirty_sectors.insert(new_state.parent);
}

//==================================================================================================
// Recomputes the render data of all sectors that got marked as dirty during this update. This
// has to happen only once all the changes are known - a sector can be processed before its
// parent even exists, and a parent can only cut out holes that are already in place.
void EditorImpl::recompute_dirty_sectors()
{
  for (EditorID sector_id : dirty_sectors)
  {
    // The void does not render anything and the sector might have been destroyed since
    if (sector_id == VOID_SECTOR_ID || !level.objects.contains(sector_id))
    {
      continue;
    }

    nc_assert(objects_render.contains(sector_id));
    EditorSectorRenderData& render_data = std::get<EditorSectorRenderData>(objects_render[sector_id]);
    this->recompute_sector_render_data(level.get_object<EditorSector>(sector_id), render_data);
  }

  dirty_sectors.clear();
}

//==================================================================================================
void EditorImpl::check_level_state_update()
{
  // React to object changes during the last frame..
  // First, check for the new and modified objects
  for (const auto&[object_id, object] : level.objects)
  {
    if (objects_mirror.contains(object_id))
    {
      // Check if it got modified..
      std::visit([&]<typename T>(const T& new_object_state)
      {
        EditorObject& second_variant = objects_mirror[object_id];
        T& old_object_state = std::get<T>(second_variant);
        if (new_object_state != old_object_state)
        {
          // Notify the change if we are interested to hear it
          if constexpr (requires {this->on_object_modified(object_id, old_object_state, new_object_state); })
          {
            this->on_object_modified(object_id, old_object_state, new_object_state);
          }

          old_object_state = new_object_state; // copy the state
        }
      }, object);
    }
    else
    {
      // The object got created!
      std::visit([&](auto& object_typed)
      {
        if constexpr (requires { this->on_object_created(object_id, object_typed); })
        {
          this->on_object_created(object_id, object_typed);
        }
      }, object);

      // Insert if not present previously
      objects_mirror.insert({object_id, object});
    }
  }

  // Then check for the deleted objects..
  for (auto it = objects_mirror.begin(); it != objects_mirror.end();)
  {
    const auto&[object_id, object] = *it;
    if (!level.objects.contains(object_id))
    {
      // The object got deleted! Propagate the information
      std::visit([&](auto& object_typed)
      {
        if constexpr (requires { this->on_object_destroyed(object_id, object_typed); })
        {
          this->on_object_destroyed(object_id, object_typed);
        }
      }, object);

      it = objects_mirror.erase(it);
    }
    else
    {
      ++it;
    }
  }

  // Now that all the changes are known, rebuild the sectors they touched
  this->recompute_dirty_sectors();
}

//==================================================================================================
void EditorImpl::update(f32 dt)
{
  this->time_since_start += cast<f64>(dt);
  this->check_level_state_update();

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

    bool has_select_tool = this->has_tool_selected<SelectTool>();
    if (ImGuiNc::ImageButton("editor_select_tool", TOOL_SIZE, has_select_tool))
    {
      this->change_tool<SelectTool>();
    }

    bool has_brush_tool = this->has_tool_selected<BrushTool>();
    if (ImGuiNc::ImageButton("editor_brush_tool", TOOL_SIZE, has_brush_tool))
    {
      this->change_tool<BrushTool>();
    }

    ImGui::EndMainMenuBar();
  }

  this->handle_dragging() || this->handle_zoom_in_out();

  std::visit([&](auto& tool)
  {
    tool.update(*this, dt);
  }, this->tool);
}

//==================================================================================================
bool EditorImpl::try_insert_line_into_map(EditorCoord start, EditorCoord end, bool& sector_created)
{
  sector_created = false;

  if (level.can_create_line(start, end))
  {
    sector_created = level.create_line(level.new_id(), start, end);
    return true;
  }

  return false;
}

//==================================================================================================
void EditorImpl::snap_to_grid(vec2& coords)
{
  coords = round(coords);
}

//==================================================================================================
vec2 EditorImpl::get_offset() const
{
  return this->center;
}

//==================================================================================================
void EditorImpl::set_offset(vec2 new_offset)
{
  constexpr vec2 LIMIT = vec2{editor::LEVEL_AREA_LIMIT, editor::LEVEL_AREA_LIMIT};
  new_offset = clamp(new_offset, -LIMIT, LIMIT);

  if (vec2 prev = this->center; prev != new_offset)
  {
    this->center = new_offset;
    this->on_offset_changed(prev, this->center);
  }
}

//==================================================================================================
void EditorImpl::on_offset_changed(vec2 /*prev*/, vec2 /*curr*/)
{
  this->recompute_grids();
}

//==================================================================================================
f32 EditorImpl::get_zoom() const
{
  return this->zoom;
}

//==================================================================================================
void EditorImpl::set_zoom(f32 new_zoom)
{
  new_zoom = clamp(new_zoom, editor::ZOOM_LIMIT_MIN, editor::ZOOM_LIMIT_MAX);

  if (f32 prev = this->zoom; prev != new_zoom)
  {
    this->zoom = new_zoom;
    this->on_zoom_changed(prev, this->zoom);
  }
}

//==================================================================================================
void EditorImpl::on_zoom_changed(f32 /*previous_zoom*/, f32 /*new_zoom*/)
{
  this->recompute_grids();
}

//==================================================================================================
void EditorImpl::recompute_exact_grid(EditorPrimitive& primitive, u64 step_size, color4 color)
{
  nc_assert(step_size > 0);

  // compute left/right/up/down world positions
  vec2 bottom_left = this->screen_to_wpos(vec2{-1.0f, -1.0f});
  vec2 top_right   = this->screen_to_wpos(vec2{ 1.0f,  1.0f});

  constexpr vec2 LIMIT = vec2{editor::LEVEL_AREA_LIMIT, editor::LEVEL_AREA_LIMIT};
  bottom_left = clamp(bottom_left, -LIMIT, LIMIT);
  top_right   = clamp(top_right,   -LIMIT, LIMIT);

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
  primitive.properties.line_width = 1.0f;
  primitive.properties.color      = color4{color.xyz, editor::GRID_ALPHA * alpha_coeff};
}

//==================================================================================================
void EditorImpl::recompute_grids()
{
  // grid sizes with different colors?
  grids.resize(NUM_GRIDS);
  for (u64 i = 0; i < NUM_GRIDS; ++i)
  {
    grids[i] = std::make_shared<EditorPrimitive>();
    this->recompute_exact_grid(*this->grids[i], GRID_SIZES[i], GRID_COLORS[i]);
  }
}

//==================================================================================================
void EditorImpl::init()
{
  this->recompute_grids();
  this->change_tool<BrushTool>();
}

//==================================================================================================
mat3 EditorImpl::calc_view_matrix()
{
  return calc_view_matrix_impl(this->get_offset(), std::exp(this->get_zoom() * 0.1f), this->aspect);
}

//==================================================================================================
vec2 EditorImpl::screen_to_wpos(vec2 screen_pos)
{
  mat3 screen_to_world = inverse(this->calc_view_matrix());
  return (screen_to_world * vec3{screen_pos, 1.0f}).xy;
}

//==================================================================================================
vec2 EditorImpl::wpos_to_screen(vec2 wpos)
{
  mat3 world_to_screen = this->calc_view_matrix();
  return (world_to_screen * vec3{wpos, 1.0f}).xy;
}

//==================================================================================================
vec2 EditorImpl::get_mouse_normalized()
{
  ImVec2 mouse   = ImGui::GetMousePos();
  ImVec2 display = ImGui::GetIO().DisplaySize;
  return {mouse.x / display.x, mouse.y / display.y};
}

//==================================================================================================
// Returns mouse coords in [-1, 1] range.
// [-1, -1] is bottom left.
vec2 EditorImpl::get_mouse_screen_pos()
{
  vec2 mpos_screen_space = this->get_mouse_normalized();
  vec2 mpos_normalized   = (mpos_screen_space * 2.0f - VEC2_ONE) * vec2{1.0f, -1.0f};
  return mpos_normalized;
}

//==================================================================================================
vec2 EditorImpl::get_mouse_wpos()
{
  return this->screen_to_wpos(this->get_mouse_screen_pos());
}

//==================================================================================================
bool EditorImpl::handle_dragging()
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

//==================================================================================================
bool EditorImpl::handle_zoom_in_out()
{
  f32 wheel = ImGui::GetIO().MouseWheel;
  wheel = floor(wheel * 10.0f) * 0.1f;

  if (!wheel)
  {
    return false;
  }

  vec2 world_pos_under_cursor_before_zoom = this->get_mouse_wpos();
  f32 new_zoom = this->get_zoom() + wheel;
  this->set_zoom(new_zoom);
  vec2 world_pos_under_cursor_after_zoom = this->get_mouse_wpos();
  vec2 difference = world_pos_under_cursor_after_zoom - world_pos_under_cursor_before_zoom;
  this->set_offset(this->get_offset() - difference);

  return true;
}

//==================================================================================================
void EditorImpl::SelectTool::modify_rendering_properties
(
  EditorPrimitiveRenderingProperties& properties,
  const EditorPrimitive&              primitive
)
{
  if (primitive.type == EditorPrimitiveType::sector && this->current_selection == SelectionType::sector)
  {
    if (primitive.sector.id == this->selection.sector.sector_id)
    {
      properties.color *= 2.0f;
    }
  }
}

//==================================================================================================
void EditorImpl::SelectTool::get_render_data(RenderList& /*list*/)
{
}

//==================================================================================================
bool EditorImpl::SelectTool::handle_dragging(EditorImpl& /*editor*/)
{
  if (this->current_selection == SelectionType::none)
  {
    return false;
  }

  bool input_allowed = !ImGui::GetIO().WantCaptureMouse;
  bool holding_right = input_allowed && ImGui::IsMouseReleased(ImGuiMouseButton_Right);

  if (holding_right != this->is_dragging)
  {
    this->is_dragging = holding_right;

    if (this->is_dragging)
    {
      // Started dragging.. Decide what do we actually want to drag
    }
    else
    {
      // Ended dragging
    }
  }

  return false;
}

//==================================================================================================
bool EditorImpl::SelectTool::handle_selection(EditorImpl& editor)
{
  EditorID sector_directly_selected = 0;
  ivec2 closest_wall_a, closest_wall_b, closest_point;

  f32 distance_to_closest_point = FLT_MAX;
  f32 distance_to_closest_wall  = FLT_MAX;

  vec2 cursor_in_world = editor.get_mouse_wpos();

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
    for (const EditorSectorRenderData::IndexList& convex_part_indices : sector.convex_parts)
    {
      u16 idx0 = convex_part_indices[0];

      for (u64 i = 0; i < convex_part_indices.size(); ++i)
      {
        u16 idx1 = convex_part_indices[ i                                  ];
        u16 idx2 = convex_part_indices[(i + 1) % convex_part_indices.size()];

        vec2 pt0 = cast<vec2>(sector.surface_points[idx0]);
        vec2 pt1 = cast<vec2>(sector.surface_points[idx1]);
        vec2 pt2 = cast<vec2>(sector.surface_points[idx2]);

        if (intersect::point_triangle(cursor_in_world, pt0, pt1, pt2))
        {
          sector_directly_selected = id;
          break;
        }
      }
    }

    // Iterate all walls and points of the sector and check their distance
    for (u64 i = 0; i < sector.walls.size(); ++i)
    {
      u64 i_next = (i + 1) % sector.walls.size();

      ivec2 pt_a = sector.walls[i     ].pt;
      ivec2 pt_b = sector.walls[i_next].pt;

      // Check point distance
      f32 pt_dist = distance(cast<vec2>(pt_a), cursor_in_world);
      if (pt_dist < distance_to_closest_point)
      {
        distance_to_closest_point = pt_dist;
        closest_point             = pt_a;
      }

      // Check wall distance
      f32 wall_dist = dist::point_line_2d(cursor_in_world, cast<vec2>(pt_a), cast<vec2>(pt_b));
      if (wall_dist < distance_to_closest_wall)
      {
        distance_to_closest_wall = wall_dist;
        closest_wall_a           = pt_a;
        closest_wall_b           = pt_b;
      }
    }
  }

  // Now evaluate what should actually be selected
  if (sector_directly_selected != 0)
  {
    this->current_selection = SelectionType::sector;
    this->selection.sector.sector_id = sector_directly_selected;
  }
  /*
  else if (distance_to_closest_point < 10.0f)
  {
    // Handle closest point
  }
  else if (distance_to_closest_wall < 10.0f)
  {
    // Handle closest wall
  }
  */
  else
  {
    this->current_selection = SelectionType::none;
  }

  return true;
}

//==================================================================================================
void EditorImpl::SelectTool::update(EditorImpl& editor, f32 /*dt*/)
{
  handle_dragging(editor) || handle_selection(editor);
}

//==================================================================================================
void EditorImpl::SelectTool::get_modifiers(RenderModifierList& list)
{
  list.push_back(this);
}

//==================================================================================================
void EditorImpl::BrushTool::update(EditorImpl& editor, f32 /*delta*/)
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
}

//==================================================================================================
void EditorImpl::BrushTool::get_render_data(RenderList& list)
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
void EditorImpl::BrushTool::get_modifiers(RenderModifierList& /*list*/)
{

}

}

#endif // #if NC_EDITOR
