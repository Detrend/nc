// Project Nuclidean Source File

#include <engine/editor/editor_sector.h>

#include <math/lingebra.h>
#include <math/utils.h>

#include <common.h>

#include <algorithm> // std::transform
#include <numeric>   // std::iota

namespace nc::editor
{
constexpr color4 SECTOR_WALL_COL    = colors::WHITE;
constexpr color4 SECTOR_SPLITS_COL  = colors::GRAY;
constexpr color4 SECTOR_SURFACE_COL = colors::NAVY;
}

namespace nc
{

//==================================================================================================
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

//==================================================================================================
struct ConvexifyPair
{
  u16 from = 0;
  u16 to   = 0;
};
// This is O(n^3) but fuck it we ball
static void convexify_sector
(
  const std::vector<ivec2>&      pts,
  const std::vector<u16>&        indices,
  std::vector<ConvexifyPair>&    pairs_out,
  std::vector<std::vector<u16>>& convex_out
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
    convexify_sector(pts, indices_a, pairs_out, convex_out);
    convexify_sector(pts, indices_b, pairs_out, convex_out);
    return;
  }

  // If we got here then everything is convex.. Copy the indices to "convex_out" and
  // exit.
  std::vector<u16>& convex_list_out = convex_out.emplace_back();
  convex_list_out.assign(indices.begin(), indices.end());
}

//==================================================================================================
void EditorSector::get_render_data(RenderList& list)
{
  if (render_data_lines->is_valid())
    list.push_back(render_data_lines->shared_from_this());

  if (render_data_surface->is_valid())
    list.push_back(render_data_surface->shared_from_this());

  if (render_data_splits->is_valid())
    list.push_back(render_data_splits->shared_from_this());
}

//==================================================================================================
void EditorSector::recompute_lines()
{
  std::vector<vec2> points;
  for (u64 i = 0; i < walls.size(); ++i)
  {
    u64 idx = i;
    u64 next_idx = (idx + 1) % walls.size();
    points.insert(points.end(), {cast<vec2>(walls[idx].pt), cast<vec2>(walls[next_idx].pt)});
  }

  render_data_lines->refresh_gpu_data(points);
  render_data_lines->properties.color = editor::SECTOR_WALL_COL;
  render_data_lines->type = EditorPrimitiveType::sector;
  render_data_lines->sector.type = 0;;
}

//==================================================================================================
void EditorSector::convexify_surface()
{
  // First allocate it into a point list
  std::vector<ivec2> points;
  std::transform(this->walls.begin(), this->walls.end(), std::back_inserter(points), [](auto&& wall)
  {
    return wall.pt;
  });

  // No need to triangulate inward sectors..
  if (!is_sector_inward(points))
  {
    return;
  }

  // Generate indices
  std::vector<u16> indices(points.size());
  std::iota(indices.begin(), indices.end(), 0_u16);

  // Convexify now
  this->convex_parts.clear();
  std::vector<ConvexifyPair> splits;
  convexify_sector(points, indices, splits, convex_parts);

  // Build lines for the convex splits (might be empty if the sector is already convex)
  std::vector<vec2> split_line_pts;
  for (auto&&[idx1, idx2] : splits)
  {
    split_line_pts.push_back(cast<vec2>(points[idx1]));
    split_line_pts.push_back(cast<vec2>(points[idx2]));
  }

  // Build triangles for the surface
  std::vector<vec2> surface_triangle_pts;
  for (const IndexList& index_list : this->convex_parts)
  {
    if (index_list.empty())
    {
      return;
    }

    vec2 pt1 = cast<vec2>(points[index_list[0]]);
    for (u64 i = 1; i < index_list.size(); ++i)
    {
      u64  prev = i - 1;
      vec2 pt2  = cast<vec2>(points[index_list[prev]]);
      vec2 pt3  = cast<vec2>(points[index_list[i   ]]);
      surface_triangle_pts.insert(surface_triangle_pts.end(), {pt1, pt2, pt3});
    }
  }

  // And refresh lines
  render_data_splits->refresh_gpu_data(split_line_pts);
  render_data_splits->properties.color = editor::SECTOR_SPLITS_COL;
  render_data_splits->type = EditorPrimitiveType::sector;
  render_data_splits->sector.type = 1;
  render_data_splits->sector.id   = this->id;

  // And surface area
  render_data_surface->refresh_gpu_data(surface_triangle_pts, GL_TRIANGLES);
  render_data_surface->properties.color = editor::SECTOR_SURFACE_COL;
  render_data_surface->type = EditorPrimitiveType::sector;
  render_data_surface->sector.type = 2;
  render_data_surface->sector.id   = this->id;
}

//==================================================================================================
void EditorSector::recompute_render_data()
{
  this->recompute_lines();
  this->convexify_surface();
}

}
