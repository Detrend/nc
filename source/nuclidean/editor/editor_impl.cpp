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
#include <math/utils.h>      // sgn

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
static void cut_holes_out_of_sector
(
  const EditorLevel&                    level,
  const EditorSector&                   sector,
  std::vector<std::vector<EditorWall>>& holes_out
)
{
  if (sector.first_hole == INVALID_EDITOR_ID)
  {
    // No holes..
    return;
  }

  // The holes are sectors of their own, we only need their outlines to cut them out of ours.
  // Beware that the hole sectors can be connected together - sharing a point or a line.

  // Algorithm: iterate the list of holes and for each one find an edge that is not shared between
  // more holes. Then from this edge trace the left most edges until we return back to it. Make a
  // hole out of all these edges and remember the IDs of holes within this enclosure to not return
  // back to them again.
  StackVector<EditorID, 16> holes_to_skip; // Skip these, already added into the list

  EditorID hole_id = sector.first_hole;
  do
  {
    EditorID curr_hole_id = hole_id;
    const EditorSector& hole = level.get_object<EditorSector>(curr_hole_id);

    hole_id = hole.next_hole;

    // Check if the hole was not already added before with others that it was connected to
    if (std::ranges::find(holes_to_skip, curr_hole_id) != holes_to_skip.end())
    {
      continue;
    }

    // Find a first line of the hole that is not shared with any other hole. If no such line
    // exists then it means that the hole is completely enclosed in between other holes.
    EditorID exclusive_half_edge_id = INVALID_EDITOR_ID; // Line that is part of only one sector
    {
      EditorID half_edge_id = hole.edge;
      do
      {
        const EditorHalfEdge& half_edge = level.get_object<EditorHalfEdge>(half_edge_id);
        const EditorHalfEdge& twin_edge = level.get_object<EditorHalfEdge>(half_edge.twin);

        if (twin_edge.sector == INVALID_EDITOR_ID)
        {
          // No sector on the other side of the line! We found the right edge for the job!
          exclusive_half_edge_id = half_edge.twin;
          break;
        }

        half_edge_id = half_edge.next;
      }
      while (half_edge_id != hole.edge);
    }

    if (exclusive_half_edge_id == INVALID_EDITOR_ID)
    {
      // This means that the hole is surrounded by other holes from each side..
      // None of it's edges will be part of the enclosure, so skip it.
      continue;
    }

    // Start a left-most iteration from the edge, which will encircle the outer loop
    f32 _;
    StackVector<EditorID, 32> enclosure_edges;
    iterate_left_most_half_edges(level, exclusive_half_edge_id, _, enclosure_edges);

    // Now append the holes into the list
    std::vector<EditorWall>& hole_walls = holes_out.emplace_back();
    std::transform(enclosure_edges.rbegin(), enclosure_edges.rend(), std::back_inserter(hole_walls),
      [&](EditorID half_edge_id) -> EditorWall
      {
        const EditorHalfEdge& half_edge = level.get_object<EditorHalfEdge>(half_edge_id);
        const EditorPoint&    point     = level.get_object<EditorPoint>(half_edge.from);
        return EditorWall{.pt = point.coords};
      });

    // And append new entries into the hole skip list
    for (EditorID half_edge_id : enclosure_edges)
    {
      const EditorHalfEdge& half_edge = level.get_object<EditorHalfEdge>(half_edge_id);
      const EditorHalfEdge& twin_edge = level.get_object<EditorHalfEdge>(half_edge.twin);

      if (std::ranges::find(holes_to_skip, twin_edge.sector) == holes_to_skip.end())
      {
        holes_to_skip.push_back(twin_edge.sector);
      }

      // This has to hold, there should be no sector on the other side
      nc_assert(half_edge.sector == INVALID_EDITOR_ID);
    }
  }
  while (hole_id != sector.first_hole);
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

  // Recompute the holes
  render_data.holes.clear();
  cut_holes_out_of_sector(this->level, sector, render_data.holes);

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
  dirty_objects.insert(object_id);
  dirty_objects.insert(sector.parent);
}

//==================================================================================================
void EditorImpl::on_object_created(EditorID object_id, const EditorPoint& point)
{
  EditorPointRenderData render_data;
  render_data.id     = object_id;
  render_data.coords = point.coords;
  this->objects_render.insert({object_id, std::move(render_data)});
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
void EditorImpl::on_object_destroyed(EditorID object_id, const EditorPoint&)
{
  objects_render.erase(object_id);
}

//==================================================================================================
void EditorImpl::on_line_modified(EditorID object_id)
{
  const EditorLine&   line = level.get_object<EditorLine>(object_id);
  const EditorHalfEdge& h1 = level.get_object<EditorHalfEdge>(line.half_edge_a);
  const EditorHalfEdge& h2 = level.get_object<EditorHalfEdge>(line.half_edge_b);
  const EditorPoint&    p1 = level.get_object<EditorPoint>(h1.from);
  const EditorPoint&    p2 = level.get_object<EditorPoint>(h2.from);

  std::get<EditorLineRenderData>(objects_render[object_id]).recreate_render_data(p1.coords, p2.coords);
}

//==================================================================================================
void EditorImpl::on_object_modified(EditorID object_id, const EditorLine&, const EditorLine&)
{
  this->on_line_modified(object_id);
}

//==================================================================================================
void EditorImpl::on_object_modified(EditorID object_id, const EditorPoint&, const EditorPoint& new_point)
{
  nc_assert(level.point_to_half_edges.contains(object_id));
  for (EditorID half_edge_id : level.point_to_half_edges[object_id])
  {
    const EditorHalfEdge& hedge = level.get_object<EditorHalfEdge>(half_edge_id);

    this->dirty_objects.insert(hedge.line);

    if (hedge.sector != INVALID_EDITOR_ID)
    {
      this->dirty_objects.insert(hedge.sector);
    }
  }

  std::get<EditorPointRenderData>(this->objects_render[object_id]).coords = new_point.coords;
}

//==================================================================================================
void EditorImpl::mark_sector_dirty(EditorID sector_id)
{
  if (sector_id == INVALID_EDITOR_ID)
  {
    return;
  }

  dirty_objects.insert(sector_id);

  if (const EditorSector* sector = level.try_get_object<EditorSector>(sector_id))
  {
    dirty_objects.insert(sector->parent);
  }
}

//==================================================================================================
void EditorImpl::on_object_created(EditorID /*object_id*/, const EditorHalfEdge& half_edge)
{
  this->mark_sector_dirty(half_edge.sector);
}

//==================================================================================================
void EditorImpl::on_object_destroyed(EditorID /*object_id*/, const EditorHalfEdge& half_edge)
{
  this->mark_sector_dirty(half_edge.sector);
}

//==================================================================================================
void EditorImpl::on_object_modified(EditorID /*object_id*/, const EditorHalfEdge& old_state, const EditorHalfEdge& new_state)
{
  this->mark_sector_dirty(old_state.sector);
  this->mark_sector_dirty(new_state.sector);
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
  dirty_objects.insert(sector.parent);
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
  dirty_objects.insert(object_id);
  dirty_objects.insert(old_state.parent);
  dirty_objects.insert(new_state.parent);
}

//==================================================================================================
void EditorImpl::recompute_dirty_objects()
{
  for (EditorID object_id : dirty_objects)
  {
    // The void does not render anything and the object might have been destroyed since
    if (object_id == VOID_SECTOR_ID || !level.objects.contains(object_id))
    {
      continue;
    }

    nc_assert(objects_render.contains(object_id));
    if (auto* render_data = std::get_if<EditorSectorRenderData>(&objects_render[object_id]))
    {
      this->recompute_sector_render_data(level.get_object<EditorSector>(object_id), *render_data);
    }

    if (std::get_if<EditorLineRenderData>(&objects_render[object_id]))
    {
      this->on_line_modified(object_id);
    }
  }

  dirty_objects.clear();
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
  this->recompute_dirty_objects();
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
      if (ImGui::MenuItem("Undo", "Ctrl+Z", nullptr, action_history.size()))
      {
        this->undo_last_action();
      }

      if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", nullptr, undone_actions.size()))
      {
        this->redo_undone_action();
      }

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

    ImGui::SetNextItemWidth(80);
    ImGui::Combo
    (
      "##Grid snap", recast<int*>(&this->snapping), GRID_SNAP_OPTION_NAMES, GRID_SNAP_OPTIONS_CNT
    );

    ImGui::EndMainMenuBar();
  }

  this->handle_keybinds();
  this->handle_dragging() || this->handle_zoom_in_out();

  std::visit([&](auto& tool)
  {
    tool.update(dt);
  }, this->tool);
}

//==================================================================================================
bool EditorImpl::try_insert_line_into_map(EditorCoord start, EditorCoord end, bool& sector_created)
{
  sector_created = false;

  if (level.can_create_line(start, end))
  {
    sector_created = do_action(ActionCreateOrDeleteLine
    {
      .create = true, .line_id = level.new_id(), .from = start, .to = end
    });

    return true;
  }

  return false;
}

//==================================================================================================
void EditorImpl::snap_to_grid(vec2& coords)
{
  f32 multiplier = cast<f32>(GRID_SNAP_OPTIONS_MULTIPLIERS[this->snapping]);
  coords = round(round(coords / multiplier) * vec2{multiplier});
}

//==================================================================================================
vec2 EditorImpl::snap_to_grid_inplace(vec2 coords)
{
  this->snap_to_grid(coords);
  return coords;
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
  return calc_view_matrix_impl(this->get_offset(), this->get_zoom_factor(), this->aspect);
}

//==================================================================================================
f32 EditorImpl::get_zoom_factor() const
{
  return std::exp(this->get_zoom() * 0.1f);
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
bool EditorImpl::is_undo_redo_allowed() const
{
  if (const SelectTool* select = std::get_if<SelectTool>(&this->tool))
  {
    // If dragging something across the screen then we should not be able to do undo/redo
    return select->is_dragging == false;
  }

  // Not dragging anythin right now
  return true;
}

//==================================================================================================
bool EditorImpl::is_delete_allowed() const
{
  return this->is_undo_redo_allowed();
}

//==================================================================================================
bool EditorImpl::handle_keybinds()
{
  bool shift = ImGui::IsKeyDown(ImGuiKey_LeftShift);
  bool ctrl  = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);
  bool key_z = ImGui::IsKeyPressed(ImGuiKey_Z);

  if (!shift && ctrl && key_z)
  {
    this->undo_last_action();
  }

  if (shift && ctrl && key_z)
  {
    this->redo_undone_action();
  }

  bool key_one = ImGui::IsKeyPressed(ImGuiKey_1);
  bool key_two = ImGui::IsKeyPressed(ImGuiKey_2);

  if (key_one)
  {
    this->change_tool<SelectTool>();
  }

  if (key_two)
  {
    this->change_tool<BrushTool>();
  }

  // Switch the grid settings
  bool key_g = ImGui::IsKeyPressed(ImGuiKey_G);
  if (key_g)
  {
    this->snapping = cast<GridSnappingOptions>((this->snapping + 1) % GridSnappingOptions::options_count);
  }

  return true;
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

  // Check the desired zoom
  this->target_zoom = clamp(this->target_zoom + wheel, editor::ZOOM_LIMIT_MIN, editor::ZOOM_LIMIT_MAX);
  if (this->get_zoom() == this->target_zoom)
  {
    return false;
  }

  vec2 world_pos_under_cursor_before_zoom = this->get_mouse_wpos();

  f32 new_zoom = (this->get_zoom() + this->target_zoom) * 0.5f;
  if (abs(new_zoom - this->target_zoom) < 0.001f)
  {
    new_zoom = this->target_zoom;
  }

  this->set_zoom(new_zoom);

  vec2 world_pos_under_cursor_after_zoom = this->get_mouse_wpos();
  vec2 difference = world_pos_under_cursor_after_zoom - world_pos_under_cursor_before_zoom;
  this->set_offset(this->get_offset() - difference);

  return true;
}

//==================================================================================================
EditorPrimitivePtr EditorImpl::get_disc_geometry() const
{
  if (!disc_geometry_primitive)
  {
    disc_geometry_primitive = std::make_shared<EditorPrimitive>();

    auto get_pt_for_angle = [](f32 angle)
    {
      return vec2{cos(angle), sin(angle)};
    };

    vec2 first_pt = get_pt_for_angle(0.0f);

    std::vector<vec2> points;

    constexpr s64 NUM_SEGMENTS = 32;
    for (s64 i = 1; i < NUM_SEGMENTS-1; ++i)
    {
      s64 i_next = i+1;
      f32 a1 = PI2 * cast<f32>(i     ) / NUM_SEGMENTS;
      f32 a2 = PI2 * cast<f32>(i_next) / NUM_SEGMENTS;

      vec2 p0 = first_pt;
      vec2 p1 = get_pt_for_angle(a1);
      vec2 p2 = get_pt_for_angle(a2);

      points.insert(points.end(), {p0, p1, p2});
    }

    disc_geometry_primitive->refresh_gpu_data(points, GL_TRIANGLES);
  }

  return disc_geometry_primitive;
}

}

#endif // #if NC_EDITOR
