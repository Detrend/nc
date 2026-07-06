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
#include <numeric>   // std::iota

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

// Maximum distance from center in all 4 directions.
constexpr s64 LEVEL_AREA_LIMIT = 512 * 48;
constexpr f32 ZOOM_LIMIT_MIN   = -100.0f;
constexpr f32 ZOOM_LIMIT_MAX   =  -10.0f;
constexpr f32 ZOOM_DEFAULT     =  -30.0f;

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
  struct ConvexifyPair
  {
    u16 from = 0;
    u16 to   = 0;
  };

  EditorPrimitivePtr      render_data_lines;
  EditorPrimitivePtr      render_data_surface;
  std::vector<EditorWall> walls;
  f32                     floor_height = 0.0f;
  f32                     ceil_height  = 0.0f;

  void get_render_data(RenderList& list)
  {
    if (render_data_lines && render_data_lines->handle.is_valid())
    {
      list.push_back(render_data_lines->shared_from_this());
    }

    if (render_data_surface && render_data_surface->handle.is_valid())
    {
      list.push_back(render_data_surface->shared_from_this());
    }
  }

  void recompute_lines()
  {
    std::vector<vec2> points;
    for (u64 i = 0; i < walls.size(); ++i)
    {
      u64 idx = i;
      u64 next_idx = (idx + 1) % walls.size();
      points.insert(points.end(), {cast<vec2>(walls[idx].pt), cast<vec2>(walls[next_idx].pt)});
    }

    render_data_lines->refresh_gpu_data(points);
  }

  // This is O(n^3) but fuck it we ball
  static void convexify_sector
  (
    const std::vector<ivec2>&       pts,
    const std::vector<u16>&         indices,
    std::vector<ConvexifyPair>& pairs_out
  )
  {
    auto get_idx_pt = [&](s64 idx)
    {
      s64 isize = cast<s64>(indices.size());
      nc_assert(idx >= -isize);
      return pts[indices[(idx + isize) % isize]];
    };

    u16 count = cast<u16>(indices.size());

    // Find a first concave point
    // If none then exit
    // Concave point found, now test intersections
    // Keep the best one
    // Report it and recurse on the 2 subsectors

    for (u16 idx = 0; idx < count; ++idx)
    {
      ivec2 before_pt = get_idx_pt(idx-1);
      ivec2 center_pt = get_idx_pt(idx+0);
      ivec2 after_pt  = get_idx_pt(idx+1);

      nc_assert(before_pt != center_pt);
      nc_assert(center_pt != after_pt);
      nc_assert(before_pt != after_pt);

      ivec2 to_before = before_pt - center_pt;
      ivec2 to_after  = after_pt  - center_pt;

      // Twice the signed area of the (before, center, after) triangle, exact in 64 bits. With
      // CCW winding a non-negative value means the polygon turns left here and the vertex is
      // convex.
      s64 turn = cast<s64>(to_after.x) * to_before.y - cast<s64>(to_after.y) * to_before.x;
      if (turn >= 0)
      {
        // This angle is convex, go on to the next point
        continue;
      }

      // This one is concave..
      // Store the best results. Sort by:
      // - distance
      // - if it splits evenly
      u16  best_idx    = idx;
      u64  best_dist2  = ~0_u64; // init as max
      bool best_splits = false;  // if it splits the concave angle onto 2 convex ones

      // Now, try find the best possible intersection
      for (u16 other_idx = 0; other_idx < count; ++other_idx)
      {
        if (other_idx == idx || (other_idx + 1) % count == idx || (idx + 1) % count == other_idx)
        {
          // Same point, previous point or the next point
          continue;
        }

        ivec2 point    = get_idx_pt(other_idx);
        ivec2 to_point = point - center_pt;

        // Sub-angles created by splitting the concave wedge with the diagonal, expressed as
        // exact 64bit cross products. Positive split_after means the CCW angle from to_after to
        // the diagonal is below 180deg, positive split_before the same for the CCW angle from
        // the diagonal to to_before.
        s64 split_after  = cast<s64>(to_after.x) * to_point.y  - cast<s64>(to_after.y) * to_point.x;
        s64 split_before = cast<s64>(to_point.x) * to_before.y - cast<s64>(to_point.y) * to_before.x;

        // The concave interior spans CCW from to_after to to_before and is over 180deg wide, so
        // the diagonal points strictly into it exactly when at least one of the sub-angles is
        // below 180deg.
        if (split_after <= 0 && split_before <= 0)
        {
          // Direction to this point is not between center->before and center->after, therefore
          // it is not suitable.
          continue;
        }

        // Cheap to do before the actual intersections
        u64 dist2 = cast<s64>(to_point.x) * to_point.x + cast<s64>(to_point.y) * to_point.y;

        // The split is nice if both resulting angles are convex (<180deg)
        bool nice_split = split_after > 0 && split_before > 0;

        if ((best_splits && !nice_split) || (best_splits == nice_split && dist2 >= best_dist2))
        {
          // The best candidate so far is at least as good - either it splits nicely and this
          // one does not, or both split the same way and the best one is closer.
          continue;
        }

        // Check that the diagonal from center to the point does not cross or touch any edge of
        // the polygon that is not incident to one of them. Touching counts as intersecting so
        // that diagonals passing exactly through another vertex (which would create zero-area
        // slivers) get rejected.
        bool intersects = false;
        for (u16 edge_idx = 0; edge_idx < count && !intersects; ++edge_idx)
        {
          u16 edge_next = cast<u16>((edge_idx + 1) % count);
          if (edge_idx == idx || edge_idx == other_idx || edge_next == idx || edge_next == other_idx)
          {
            // Edges sharing an endpoint with the diagonal always touch it there
            continue;
          }

          ivec2 e1 = get_idx_pt(edge_idx);
          ivec2 e2 = get_idx_pt(edge_next);

          // Twice the signed triangle areas, exact in 64 bits. The sign says on which side of
          // the diagonal (or edge) the tested point lies, zero means it is collinear.
          s64 e1_side = cast<s64>(to_point.x)  * (e1.y - center_pt.y) - cast<s64>(to_point.y)  * (e1.x - center_pt.x);
          s64 e2_side = cast<s64>(to_point.x)  * (e2.y - center_pt.y) - cast<s64>(to_point.y)  * (e2.x - center_pt.x);
          s64 c_side  = cast<s64>(e2.x - e1.x) * (center_pt.y - e1.y) - cast<s64>(e2.y - e1.y) * (center_pt.x - e1.x);
          s64 p_side  = cast<s64>(e2.x - e1.x) * (point.y     - e1.y) - cast<s64>(e2.y - e1.y) * (point.x     - e1.x);

          // Proper crossing - edge endpoints strictly on opposite sides of the diagonal and
          // diagonal endpoints strictly on opposite sides of the edge
          bool crosses_diag = (e1_side > 0 && e2_side < 0) || (e1_side < 0 && e2_side > 0);
          bool crosses_edge = (c_side  > 0 && p_side  < 0) || (c_side  < 0 && p_side  > 0);

          // Touching - an endpoint of one segment collinear with and inside the bounding box of
          // the other segment. Covers collinear overlaps as well.
          bool e1_touches = e1_side == 0
                         && min(center_pt.x, point.x) <= e1.x && e1.x <= max(center_pt.x, point.x)
                         && min(center_pt.y, point.y) <= e1.y && e1.y <= max(center_pt.y, point.y);
          bool e2_touches = e2_side == 0
                         && min(center_pt.x, point.x) <= e2.x && e2.x <= max(center_pt.x, point.x)
                         && min(center_pt.y, point.y) <= e2.y && e2.y <= max(center_pt.y, point.y);
          bool c_touches  = c_side == 0
                         && min(e1.x, e2.x) <= center_pt.x && center_pt.x <= max(e1.x, e2.x)
                         && min(e1.y, e2.y) <= center_pt.y && center_pt.y <= max(e1.y, e2.y);
          bool p_touches  = p_side == 0
                         && min(e1.x, e2.x) <= point.x && point.x <= max(e1.x, e2.x)
                         && min(e1.y, e2.y) <= point.y && point.y <= max(e1.y, e2.y);

          intersects = (crosses_diag && crosses_edge) || e1_touches || e2_touches || c_touches || p_touches;
        }

        // Check if we found a better point we can connect to
        if (!intersects)
        {
          best_dist2  = dist2;
          best_idx    = other_idx;
          best_splits = nice_split;
        }
      }

      // Can happen for degenerate polygons
      nc_assert(best_idx != idx, "No valid split from a concave vertex - invalid polygon?");

      // Report the split
      pairs_out.push_back(ConvexifyPair{indices[idx], indices[best_idx]});

      // Split the indices into 2 groups
      std::vector<u16> indices_a;
      std::vector<u16> indices_b;

      u16 s1 = min(idx, best_idx);
      u16 s2 = max(idx, best_idx);
      nc_assert(s1 != s2);

      for (u64 idx_idx = 0; idx_idx < indices.size(); ++idx_idx)
      {
        if (idx_idx < s1)
        {
          indices_a.push_back(indices[idx_idx]);
        }
        else if (idx_idx == s1)
        {
          indices_a.push_back(indices[idx_idx]);
          indices_b.push_back(indices[idx_idx]);
        }
        else if (idx_idx > s1 && idx_idx < s2)
        {
          indices_b.push_back(indices[idx_idx]);
        }
        else if (idx_idx == s2)
        {
          indices_a.push_back(indices[idx_idx]);
          indices_b.push_back(indices[idx_idx]);
        }
        else if (idx_idx > s2)
        {
          indices_a.push_back(indices[idx_idx]);
        }
        else
        {
          nc_assert(false, "Should not happen!");
        }
      }

      // And run on each group recursively
      convexify_sector(pts, indices_a, pairs_out);
      convexify_sector(pts, indices_b, pairs_out);
      return;
    }
  }

  static bool is_sector_inward(const std::vector<ivec2>& pts)
  {
    f32 degs = 0.0f;

    for (u64 idx = 0; idx < pts.size(); ++idx)
    {
      u64 idx2 = (idx  + 1) % pts.size();
      u64 idx3 = (idx2 + 1) % pts.size();
      ivec2 a = pts[idx];
      ivec2 b = pts[idx2];
      ivec2 c = pts[idx3];

      nc_assert(a != b);
      nc_assert(b != c);
      nc_assert(a != c);

      vec2 dir1 = normalize(cast<vec2>(b - a));
      vec2 dir2 = normalize(cast<vec2>(c - b));
      f32  sign = sgn(cross(dir1, dir2));
      f32  angle_rad = acos(dot(dir1, dir2)) * sign;
      degs += rad2deg(angle_rad);
    }

    return is_zero(degs - 360.0f, 0.01f);
  }

  void convexify_surface()
  {
    std::vector<ivec2> pts;
    std::transform(walls.begin(), walls.end(), std::back_inserter(pts), [](const EditorWall& wall)
    {
      return wall.pt;
    });

    // No need to triangulate inward sectors..
    if (!is_sector_inward(pts))
    {
      return;
    }

    std::vector<u16> indices(pts.size());
    std::iota(indices.begin(), indices.end(), 0_u16);

    std::vector<ConvexifyPair> splits;

    convexify_sector(pts, indices, splits);

    std::vector<vec2> split_line_pts;
    for (const auto&[idx1, idx2] : splits)
    {
      split_line_pts.push_back(cast<vec2>(pts[idx1]));
      split_line_pts.push_back(cast<vec2>(pts[idx2]));
    }

    render_data_surface->refresh_gpu_data(split_line_pts);
    render_data_surface->color = colors::GRAY;
  }

  void recompute_render_data()
  {
    if (!render_data_lines)
    {
      render_data_lines = std::make_shared<EditorPrimitive>();
    }

    if (!render_data_surface)
    {
      render_data_surface = std::make_shared<EditorPrimitive>();
    }

    this->recompute_lines();
    this->convexify_surface();
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
  f32                             zoom   = editor::ZOOM_DEFAULT;
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

      /*
      bool has_entity_tool = this->has_tool_selected<EmptyTool>();
      if (ImGuiNc::ImageButton("editor_entity_tool", TOOL_SIZE, has_entity_tool))
      {
        
      }
      */

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
      std::transform(pts.begin(), pts.end()-1, std::back_inserter(new_sector.walls), [&](ivec2 point)
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

  // Enables selection and movement of walls, sectors and entities.
  struct SelectTool
  {
    void get_render_data(RenderList& /*list*/)
    {
      
    }

    void update(EditorImpl& /*editor*/, f32 /*dt*/)
    {
      
    }
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
      constexpr vec2 CURSOR_DIRS[4] = {VEC2_X, VEC2_Y, -VEC2_X, -VEC2_Y};
      std::vector<vec2> cursor_pts;
      for (u64 i = 0; i < 4; ++i)
      {
        vec2 a = mouse_world_pos + cursor_len * CURSOR_DIRS[i];
        vec2 b = mouse_world_pos + cursor_len * CURSOR_DIRS[(i+1) % 4];
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

  std::variant<SelectTool, BrushTool> tool;

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
    constexpr vec2 LIMIT = vec2{editor::LEVEL_AREA_LIMIT, editor::LEVEL_AREA_LIMIT};
    new_offset = clamp(new_offset, -LIMIT, LIMIT);

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
    new_zoom = clamp(new_zoom, editor::ZOOM_LIMIT_MIN, editor::ZOOM_LIMIT_MAX);

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

      /*
      x_start = clamp(x_start, -editor::LEVEL_AREA_LIMIT, editor::LEVEL_AREA_LIMIT);
      x_end   = clamp(x_end,   -editor::LEVEL_AREA_LIMIT, editor::LEVEL_AREA_LIMIT);
      y_start = clamp(y_start, -editor::LEVEL_AREA_LIMIT, editor::LEVEL_AREA_LIMIT);
      y_end   = clamp(y_end,   -editor::LEVEL_AREA_LIMIT, editor::LEVEL_AREA_LIMIT);
      */

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
    f32 new_zoom = this->get_zoom() + wheel;
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
