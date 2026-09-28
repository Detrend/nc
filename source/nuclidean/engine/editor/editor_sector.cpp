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
static bool does_segment_hit_edge(ivec2 a, ivec2 b, ivec2 e1, ivec2 e2)
{
  bool e1_at_end = e1 == a || e1 == b;
  bool e2_at_end = e2 == a || e2 == b;

  if (e1_at_end && e2_at_end)
  {
    // The edge is the segment itself
    return true;
  }

  ivec2 dir = b - a;

  // Twice the signed triangle areas, exact in 64 bits. The sign says on which side of the segment
  // (or edge) the tested point lies, zero means it is collinear.
  s64 e1_side = cast<s64>(dir.x)       * (e1.y - a.y)  - cast<s64>(dir.y)       * (e1.x - a.x);
  s64 e2_side = cast<s64>(dir.x)       * (e2.y - a.y)  - cast<s64>(dir.y)       * (e2.x - a.x);
  s64 a_side  = cast<s64>(e2.x - e1.x) * (a.y  - e1.y) - cast<s64>(e2.y - e1.y) * (a.x  - e1.x);
  s64 b_side  = cast<s64>(e2.x - e1.x) * (b.y  - e1.y) - cast<s64>(e2.y - e1.y) * (b.x  - e1.x);

  // Proper crossing - edge endpoints strictly on opposite sides of the segment and segment
  // endpoints strictly on opposite sides of the edge
  bool crosses_segment = (e1_side > 0 && e2_side < 0) || (e1_side < 0 && e2_side > 0);
  bool crosses_edge    = (a_side  > 0 && b_side  < 0) || (a_side  < 0 && b_side  > 0);

  // Touching - an endpoint of one of them collinear with and inside the bounding box of the other
  // one. Covers collinear overlaps as well.
  bool e1_touches = !e1_at_end && e1_side == 0
                 && min(a.x, b.x) <= e1.x && e1.x <= max(a.x, b.x)
                 && min(a.y, b.y) <= e1.y && e1.y <= max(a.y, b.y);
  bool e2_touches = !e2_at_end && e2_side == 0
                 && min(a.x, b.x) <= e2.x && e2.x <= max(a.x, b.x)
                 && min(a.y, b.y) <= e2.y && e2.y <= max(a.y, b.y);
  bool a_touches  = a != e1 && a != e2 && a_side == 0
                 && min(e1.x, e2.x) <= a.x && a.x <= max(e1.x, e2.x)
                 && min(e1.y, e2.y) <= a.y && a.y <= max(e1.y, e2.y);
  bool b_touches  = b != e1 && b != e2 && b_side == 0
                 && min(e1.x, e2.x) <= b.x && b.x <= max(e1.x, e2.x)
                 && min(e1.y, e2.y) <= b.y && b.y <= max(e1.y, e2.y);

  return (crosses_segment && crosses_edge) || e1_touches || e2_touches || a_touches || b_touches;
}

//==================================================================================================
// Checks if a direction points strictly into the interior of a polygon at one of its vertices. The
// interior is expected on the left of the polygon, so it spans counter clockwise from the direction
// towards the next point to the direction towards the previous one.
static bool points_into_wedge(ivec2 to_before, ivec2 to_after, ivec2 dir)
{
  s64 turn         = cast<s64>(to_after.x) * to_before.y - cast<s64>(to_after.y) * to_before.x;
  s64 after_side   = cast<s64>(to_after.x) * dir.y       - cast<s64>(to_after.y) * dir.x;
  s64 before_side  = cast<s64>(dir.x)      * to_before.y - cast<s64>(dir.y)      * to_before.x;

  if (turn > 0)
  {
    // Convex vertex, the wedge is under 180 degrees and we have to be inside both of its halves
    return after_side > 0 && before_side > 0;
  }

  if (turn < 0)
  {
    // Concave vertex, the wedge is over 180 degrees and being inside one of the halves is enough
    return after_side > 0 || before_side > 0;
  }

  // The polygon goes straight through, so the wedge is the half plane on the left
  return after_side > 0;
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

  // Once the sector has holes, the points where a hole got connected to the rest appear in the list
  // more than once (see convexify_surface).
  auto is_repeated = [&](u16 local_idx)
  {
    ivec2 pt = get_idx_pt(local_idx);
    for (u16 i = 0; i < count; ++i)
    {
      if (i != local_idx && get_idx_pt(i) == pt)
      {
        return true;
      }
    }

    return false;
  };

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

      if (point == center_pt)
      {
        // Another copy of the very same point, which can only happen once the sector has holes
        continue;
      }

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

      // Each copy of a repeated point borders only its own part of the area around that point.
      // The diagonal has to arrive into the part belonging to this copy, otherwise splitting here
      // would tear the polygon apart. For a point that appears only once this always holds
      // already, so there is no need to check it.
      if (is_repeated(other_idx))
      {
        ivec2 other_to_before = get_idx_pt(other_idx - 1) - point;
        ivec2 other_to_after  = get_idx_pt(other_idx + 1) - point;
        if (!points_into_wedge(other_to_before, other_to_after, center_pt - point))
        {
          continue;
        }
      }

      // Check that the diagonal from center to the point does not cross or touch any edge of
      // the polygon that is not incident to one of them.
      bool intersects = false;
      for (u16 edge_idx = 0; edge_idx < count && !intersects; ++edge_idx)
      {
        u16 edge_next = cast<u16>((edge_idx + 1) % count);
        if (edge_idx == idx || edge_idx == other_idx || edge_next == idx || edge_next == other_idx)
        {
          // Edges sharing an endpoint with the diagonal always touch it there
          continue;
        }

        intersects = does_segment_hit_edge(center_pt, point, get_idx_pt(edge_idx), get_idx_pt(edge_next));
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
void EditorSectorRenderData::get_render_data(RenderList& list)
{
  if (render_data_lines->is_valid())
    list.push_back(render_data_lines->shared_from_this());

  if (render_data_surface->is_valid())
    list.push_back(render_data_surface->shared_from_this());

  if (render_data_splits->is_valid())
    list.push_back(render_data_splits->shared_from_this());
}

//==================================================================================================
void EditorSectorRenderData::recompute_lines()
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
void EditorSectorRenderData::convexify_surface()
{
  // First allocate it into a point list
  std::vector<ivec2> points;
  std::transform(this->walls.begin(), this->walls.end(), std::back_inserter(points), [](auto&& wall)
  {
    return wall.pt;
  });

  nc_assert(is_sector_inward(points));

  // Generate indices
  std::vector<u16> indices(points.size());
  std::iota(indices.begin(), indices.end(), 0_u16);

  std::vector<ConvexifyPair> splits;

  if (!this->holes.empty())
  {
    // Hole points go after the outer ones. The holes are stored counter clockwise, but the loop
    // has to go around them clockwise so that the area of the sector stays on its left.
    std::vector<std::vector<u16>> hole_loops;
    for (const std::vector<EditorWall>& hole : this->holes)
    {
      std::vector<ivec2> hole_points;
      std::transform(hole.begin(), hole.end(), std::back_inserter(hole_points), [](auto&& wall)
      {
        return wall.pt;
      });

      nc_assert(is_sector_inward(hole_points));

      std::vector<u16>& hole_loop = hole_loops.emplace_back(hole_points.size());
      std::iota(hole_loop.begin(), hole_loop.end(), cast<u16>(points.size()));
      std::reverse(hole_loop.begin(), hole_loop.end());

      points.insert(points.end(), hole_points.begin(), hole_points.end());
    }

    // Take the holes from the right to the left. Nothing that is still unconnected then lies to
    // the right of the rightmost point of the current hole, so the loop is always visible from it
    // and a bridge always exists.
    auto rightmost_x = [&](const std::vector<u16>& loop)
    {
      s32 max_x = points[loop.front()].x;
      for (u16 point_idx : loop)
      {
        max_x = max(max_x, points[point_idx].x);
      }
      return max_x;
    };

    std::sort(hole_loops.begin(), hole_loops.end(), [&](const auto& a, const auto& b)
    {
      return rightmost_x(a) > rightmost_x(b);
    });

    for (u64 hole_idx = 0; hole_idx < hole_loops.size(); ++hole_idx)
    {
      const std::vector<u16>& hole_loop = hole_loops[hole_idx];

      // Try all pairs of a loop point and a hole point and keep the shortest bridge that goes
      // through the sector without touching anything. That is O(n^3) as well, but sectors are
      // small.
      u64 best_loop_pos = ~0_u64;
      u64 best_hole_pos = ~0_u64;
      u64 best_dist2    = ~0_u64;

      for (u64 loop_pos = 0; loop_pos < indices.size(); ++loop_pos)
      {
        ivec2 loop_pt     = points[indices[loop_pos]];
        ivec2 loop_before = points[indices[(loop_pos + indices.size() - 1) % indices.size()]];
        ivec2 loop_after  = points[indices[(loop_pos + 1) % indices.size()]];

        for (u64 hole_pos = 0; hole_pos < hole_loop.size(); ++hole_pos)
        {
          ivec2 hole_pt     = points[hole_loop[hole_pos]];
          ivec2 hole_before = points[hole_loop[(hole_pos + hole_loop.size() - 1) % hole_loop.size()]];
          ivec2 hole_after  = points[hole_loop[(hole_pos + 1) % hole_loop.size()]];

          if (hole_pt == loop_pt)
          {
            continue;
          }

          ivec2 to_hole = hole_pt - loop_pt;
          u64   dist2   = cast<s64>(to_hole.x) * to_hole.x + cast<s64>(to_hole.y) * to_hole.y;
          if (dist2 >= best_dist2)
          {
            continue;
          }

          // The bridge has to leave the loop into the sector and enter the hole from the outside
          if (!points_into_wedge(loop_before - loop_pt, loop_after - loop_pt, to_hole) ||
              !points_into_wedge(hole_before - hole_pt, hole_after - hole_pt, -to_hole))
          {
            continue;
          }

          // And it must not hit the loop or any of the holes, including this one
          auto hits_any_edge = [&](const std::vector<u16>& edges_loop)
          {
            for (u64 i = 0; i < edges_loop.size(); ++i)
            {
              ivec2 e1 = points[edges_loop[i]];
              ivec2 e2 = points[edges_loop[(i + 1) % edges_loop.size()]];
              if (does_segment_hit_edge(loop_pt, hole_pt, e1, e2))
              {
                return true;
              }
            }
            return false;
          };

          bool blocked = hits_any_edge(indices);
          for (u64 other_hole = hole_idx; other_hole < hole_loops.size() && !blocked; ++other_hole)
          {
            blocked = hits_any_edge(hole_loops[other_hole]);
          }

          if (!blocked)
          {
            best_loop_pos = loop_pos;
            best_hole_pos = hole_pos;
            best_dist2    = dist2;
          }
        }
      }

      nc_assert(best_loop_pos != ~0_u64, "No bridge from a hole to the rest of the sector - invalid holes?");

      // Splice the hole in right after the loop point: across the bridge, around the whole hole
      // starting from its end of the bridge, back to that end once more, back across the bridge
      // and on along the loop.
      u16 loop_point = indices[best_loop_pos];
      u16 hole_point = hole_loop[best_hole_pos];

      std::vector<u16> spliced;
      spliced.reserve(indices.size() + hole_loop.size() + 2);
      spliced.insert(spliced.end(), indices.begin(), indices.begin() + best_loop_pos + 1);
      spliced.insert(spliced.end(), hole_loop.begin() + best_hole_pos, hole_loop.end());
      spliced.insert(spliced.end(), hole_loop.begin(), hole_loop.begin() + best_hole_pos);
      spliced.insert(spliced.end(), {hole_point, loop_point});
      spliced.insert(spliced.end(), indices.begin() + best_loop_pos + 1, indices.end());
      indices = std::move(spliced);

      // The bridge separates the convex parts on its two sides just like any other split does
      splits.push_back(ConvexifyPair{loop_point, hole_point});
    }
  }

  // The convex parts index into these
  this->surface_points = points;

  // Convexify now
  this->convex_parts.clear();
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
  render_data_splits->order       = 10;

  // And surface area
  render_data_surface->refresh_gpu_data(surface_triangle_pts, GL_TRIANGLES);
  render_data_surface->properties.color = editor::SECTOR_SURFACE_COL;
  render_data_surface->type = EditorPrimitiveType::sector;
  render_data_surface->sector.type = 2;
  render_data_surface->sector.id   = this->id;
}

//==================================================================================================
void EditorSectorRenderData::recompute_render_data()
{
  //this->recompute_lines();
  this->convexify_surface();
}

}
