// Project Nucledian Source File
#include <intersect.h>
#include <common.h>

#include <aabb.h>

#include <math/utils.h>
#include <math/vector.h>
#include <math/lingebra.h>

#include <limits>     // FLT_MAX
#include <algorithm>  // std::min, std::max, std::abs
#include <array>
#include <cmath>

namespace nc::intersect
{

//==============================================================================
bool segment_segment(
  vec2 start_a,
  vec2 end_a,
  vec2 start_b,
  vec2 end_b,
  f32& t_out,
  f32& u_out)
{
  constexpr f32 TOLERANCE = 0.0001f;   // 0.1mm

  t_out = FLT_MAX;
  u_out = FLT_MAX;

  NC_ASSERT(start_a != end_a);
  NC_ASSERT(start_b != end_b);

  const vec2 dir_a = end_a - start_a;
  const vec2 dir_b = end_b - start_b;
  const f32  top = cross(dir_b, start_b - start_a);
  const f32  bottom = cross(dir_b, dir_a);

  const bool tp_zero = is_zero(top, TOLERANCE);
  const bool bt_zero = is_zero(bottom, TOLERANCE);

  if (tp_zero && bt_zero)
  {
    // parallel and lines are intersecting, check for segment
    // intersection by projecting onto an interval
    // TODO: can be probably done faster
    // TODO: maybe no need to normalize?
    const vec2 projection_plane = /*normalize(*/dir_a/*)*/;

    const f32 a1 = dot(start_a, projection_plane);
    const f32 a2 = dot(end_a, projection_plane);
    const f32 b1 = dot(start_b, projection_plane);
    const f32 b2 = dot(end_b, projection_plane);

    const f32 imin = std::max(std::min(a1, a2), std::min(b1, b2));
    const f32 imax = std::min(std::max(a1, a2), std::max(b1, b2));

    return imax >= imin;
  }
  else if (bt_zero)
  {
    // parallel and non-intersecting
    return false;
  }
  else
  {
    const f32 t = top / bottom;

    // might be intersecting, check for u as well
    const f32 utop = cross(dir_a, start_a - start_b);
    const f32 ubottom = -bottom;

    // This should not happen AFAIK
    NC_ASSERT(!is_zero(ubottom, TOLERANCE));

    const f32 u = utop / ubottom;

    t_out = t;
    u_out = u;

    // lines intersect, but segments do not
    return t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f;
  }
}

//==============================================================================
bool aabb_aabb_2d(const aabb2& a, const aabb2& b)
{
  vec2 mn = max(a.min, b.min);
  vec2 mx = min(a.max, b.max);
  aabb2 new_aabb;
  new_aabb.min = mn;
  new_aabb.max = mx;
  return new_aabb.is_valid();
}

//==============================================================================
bool point_triangle(vec2 p, vec2 a, vec2 b, vec2 c)
{
  const auto a_to_b = b - a;
  const auto b_to_c = c - b;
  const auto c_to_a = a - c;

  const auto a_to_p = p - a;
  const auto b_to_p = p - b;
  const auto c_to_p = p - c;

  const f32 gl_sign = sgn(cross(a_to_b, b_to_c));
  if (gl_sign == 0.0f)
  {
    return false;
  }

  const f32 a_sign = sgn(cross(a_to_b, a_to_p));
  const f32 b_sign = sgn(cross(b_to_c, b_to_p));
  const f32 c_sign = sgn(cross(c_to_a, c_to_p));

  auto eq_or_zero = [](f32 f1, f32 f2)
  {
    return f1 == f2 || f2 == 0.0f;
  };

  return eq_or_zero(gl_sign, a_sign)
    && eq_or_zero(gl_sign, b_sign)
    && eq_or_zero(gl_sign, c_sign);
}

//==============================================================================
bool triangle_triangle(vec2 a1, vec2 b1, vec2 c1, vec2 a2, vec2 b2, vec2 c2)
{
  auto arr1 = std::array{ a1, b1, c1 };
  auto arr2 = std::array{ a2, b2, c2 };
  return convex_convex(
    std::span{ arr1.data(), arr1.size() },
    std::span{ arr2.data(), arr2.size() });
}

//==============================================================================
bool convex_convex(std::span<vec2> a, std::span<vec2> b, f32 threshold)
{
  // iterate edges of both shapes and search for a line that separates
  // both of them
  const std::span<vec2> points[2] = { a, b };

  // shapes must have at least 3 points
  if (points[0].size() < 3 || points[1].size() < 3)
  {
    return false;
  }

  for (u8 shape_idx = 0; shape_idx < 2; ++shape_idx)
  {
    for (u8 side_idx = 0; side_idx < points[shape_idx].size(); ++side_idx)
    {
      const u8 next_side_idx = (side_idx + 1) % points[shape_idx].size();

      const auto& p1 = points[shape_idx][side_idx];
      const auto& p2 = points[shape_idx][next_side_idx];
      const auto  line = normalize(p1 - p2);
      const auto  normal = vec2{ line.y, -line.x };

      struct LocalInterval
      {
        f32 min = FLT_MAX;
        f32 max = -FLT_MAX;   // FLT_MIN is a trap..
      };

      LocalInterval intervals[2]{};

      // now project all points onto the normal and create
      // an interval for each of the two triangles - if the
      // intervals overlap then the triangles MIGHT overlap,
      // but we must check all other hyperplanes as well
      for (u8 pset_idx = 0; pset_idx < 2; ++pset_idx)
      {
        for (u8 point_idx = 0; point_idx < points[pset_idx].size(); ++point_idx)
        {
          const f32 projected = dot(points[pset_idx][point_idx], normal);

          // update min/max of the interval
          auto& interval = intervals[pset_idx];
          interval.min = std::min(interval.min, projected);
          interval.max = std::max(interval.max, projected);
        }
      }

      const f32 interval_min = std::max(intervals[0].min, intervals[1].min);
      const f32 interval_max = std::min(intervals[0].max, intervals[1].max);

      // if the shapes have an intersection smaller than 1cm
      // then we pretend they do not intersect
      if (interval_max - interval_min < threshold)
      {
        // if we got here than it means that we found a hyperplane that separates
        // both triangles from each other and therefore they do not intersect
        return false;
      }

      // plane not found, go on
      // TODO: finish this!!!
    }
  }

  // there is no hyperplane that separates the two shapes, therefore
  // they must intersect
  return true;
}

//==============================================================================
bool segment_circle(vec2 start, vec2 end, vec2 og_center, f32 r, f32& t_out, vec2& n_out)
{
  NC_TODO("Circle collision might not work properly if the raycasted point is inside the circle.");

  NC_ASSERT(start != end);
  NC_ASSERT(r > 0.0f);

  const vec2 center = og_center - start;
  const vec2 dir = end - start;

  const f32 dx = dir.x;
  const f32 dy = dir.y;
  const f32 cx = center.x;
  const f32 cy = center.y;
  const f32 rr = r * r;

  const f32 a = dx * dx + dy * dy;
  const f32 b = -2 * (dx * cx + dy * cy);
  const f32 c = -1 * (rr - cx * cx - cy * cy);
  NC_ASSERT(a != 0.0f);

  const f32 D2 = b * b - 4 * a * c;
  if (D2 < 0.0f)
  {
    // no solution
    return false;
  }

  const f32 a2 = 2 * a;
  const f32 D = std::sqrtf(D2);

  const f32 t1 = (-b + D) / a2;
  const f32 t2 = (-b - D) / a2;

  const bool t1_legit = t1 == std::clamp(t1, 0.0f, 1.0f);
  const bool t2_legit = t2 == std::clamp(t2, 0.0f, 1.0f);

  if (!t1_legit && !t2_legit)
  {
    // the circle is too far away or not intersected
    return false;
  }

  f32 t;
  if (t1_legit && t2_legit)
  {
    t = std::min(t1, t2);
  }
  else if (t1_legit)
  {
    t = t1;
  }
  else
  {
    t = t2;
  }

  const vec2 col_point = dir * t;
  NC_ASSERT(col_point != center);
  const vec2 center_to_col = col_point - center;

  t_out = t;
  n_out = normalize(center_to_col);
  return true;
}

//==============================================================================
bool segment_segment_expanded(
  vec2  start_a,
  vec2  end_a,
  vec2  start_b,
  vec2  end_b,
  f32   expand_b,
  vec2& out_normal,
  f32& out_coeff)
{
  NC_ASSERT(start_a != end_a);
  NC_ASSERT(start_b != end_b);
  NC_ASSERT(expand_b > 0.0f); // use normal intersection if the parameter is 0

  f32  cs_coeff = 0, ce_coeff = 0;
  f32 ll_coeff, lr_coeff, _;
  vec2 cs_n, ce_n;

  const vec2 b_norm = flipped(normalize(end_b - start_b));
  const vec2 b_offset = b_norm * expand_b;

  const bool cs_intersect = segment_circle(start_a, end_a, start_b, expand_b, cs_coeff, cs_n);
  const bool ce_intersect = segment_circle(start_a, end_a, end_b, expand_b, ce_coeff, ce_n);
  const bool ll_intersect = segment_segment(start_a, end_a, start_b + b_offset, end_b + b_offset, ll_coeff, _);
  const bool lr_intersect = segment_segment(start_a, end_a, start_b - b_offset, end_b - b_offset, lr_coeff, _);

  out_normal = vec2{ 0 };
  out_coeff = FLT_MAX;

  auto take_best = [&](bool intersection, f32 coeff, vec2 norm)
  {
    if (intersection && coeff < out_coeff)
    {
      out_coeff = coeff;
      out_normal = norm;
    }
  };

  take_best(cs_intersect, cs_coeff, cs_n);
  take_best(ce_intersect, ce_coeff, ce_n);
  take_best(ll_intersect, ll_coeff, b_norm);
  take_best(lr_intersect, lr_coeff, -b_norm);

  return out_coeff != FLT_MAX;
}

//==============================================================================
bool ray_wall(
  vec3  ray_start,
  vec3  ray_end,
  vec2  wall_a,
  vec2  wall_b,
  f32   wall_y1,
  f32   wall_y2,
  f32&  out_coeff)
{
  NC_ASSERT(wall_y1   != wall_y2, "Points of the wall are the same.");
  NC_ASSERT(ray_start != ray_end, "Ray has a zero length.");

  const vec2 start2d = ray_start.xz;
  const vec2 end2d   = ray_end.xz;
  const vec3 ray_dir = ray_end - ray_start;

  f32 _;
  if (!segment_segment(start2d, end2d, wall_a, wall_b, out_coeff, _))
  {
    // not intersecting even in 2D, can't intersect in 3D
    return false;
  }

  // if we intersect in 2D then check the Y height of the intersection
  // point and check if it is on the wall
  const vec3 intersect_point = ray_start + ray_dir * out_coeff;
  const f32  y_bot = std::min(wall_y1, wall_y2);
  const f32  y_top = std::max(wall_y1, wall_y2);

  return intersect_point.y >= y_bot && intersect_point.y <= y_top;
}

//==============================================================================
namespace helpers
{

//==============================================================================
template<typename SWIZZLE>
bool ray_infinite_axis_aligned_plane(
  vec3    ray_start,
  vec3    ray_end,
  f32     plane_coord,
  f32&    out_coeff,
  SWIZZLE swizzler)
{
  NC_ASSERT(ray_start != ray_end);
  const vec3 ray_dir = ray_end - ray_start;

  // check if the ray is horizontal
  if (swizzler(ray_dir) == 0.0f) [[unlikely]]
  {
    // check if the ray starts at the same height as the plane
    if (swizzler(ray_start) == plane_coord) [[unlikely]]
    {
      out_coeff = 0.0f;
      return true;
    }
    else
    {
      return false;
    }
  }

  const f32 plane_coord_adjusted = plane_coord - swizzler(ray_start);

  // we are searching for parameter t such that
  //   ray_dir.y * t = plane_y_adjusted
  // therefore..
  //   t = plane_y_adjusted / ray_dir.y
  // and we have a guarantee that y is not zero
  const f32 t = plane_coord_adjusted / swizzler(ray_dir);
  if (t >= 0.0f && t <= 1.0f)
  {
    out_coeff = t;
    return true;
  }
  else
  {
    return false;
  }
}

}

//==============================================================================
bool ray_infinite_plane_xz(
  vec3 ray_start,
  vec3 ray_end,
  f32  plane_y,
  f32& out_coeff)
{
  auto swizzle_y = [](const vec3& v) { return v.y; };
  return helpers::ray_infinite_axis_aligned_plane
  (
    ray_start,
    ray_end,
    plane_y,
    out_coeff,
    swizzle_y
  );
}

//==============================================================================
bool ray_aabb3(
  vec3  ray_start,
  vec3  ray_end,
  aabb3 bbox,
  f32&  out_coeff,
  vec3& out_normal)
{
  if (!bbox.is_valid()) [[unlikely]]
  {
    return false;
  }

  using Swizzler = f32(*)(const vec3&);
  constexpr Swizzler swizzlers[3] =
  {
    [](const vec3& v) { return v.x; },
    [](const vec3& v) { return v.y; },
    [](const vec3& v) { return v.z; },
  };

  constexpr vec3 axes[3] = {VEC3_X, VEC3_Y, VEC3_Z};

  f32  closest_hit = FLT_MAX;
  vec3 hit_normal  = vec3{0};

  auto add_possible_hit = [&](f32 coeff, vec3 normal)
  {
    if (coeff >= closest_hit || coeff < 0.0f || coeff > 1.0f)
    {
      return;
    }

    const vec3 hit_pt  = ray_start + (ray_end - ray_start) * coeff;
    const vec3 in_bbox = clamp(hit_pt, bbox.min, bbox.max);

    // Check if we actually hit the box itself. Threshold for float inaccuracies
    if (is_zero(in_bbox - hit_pt, 0.001f))
    {
      closest_hit = coeff;
      hit_normal  = normal;
    }
  };

  for (u8 i = 0; i < 3; ++i)
  {
    f32   intersect_coord = 0.0f;
    vec3  normal          = vec3{0};
    auto& swizzler        = swizzlers[i];

    if (swizzler(ray_start) <= swizzler(bbox.min))
    {
      // use the min value
      intersect_coord = swizzler(bbox.min);
      normal = -axes[i];
    }
    else if (swizzler(ray_start) >= swizzler(bbox.max))
    {
      // use the max value
      intersect_coord = swizzler(bbox.max);
      normal = axes[i];
    }
    else
    {
      // non of these can be intersected
      continue;
    }

    f32 out;
    const bool hit = helpers::ray_infinite_axis_aligned_plane
    (
      ray_start,
      ray_end,
      intersect_coord,
      out,
      swizzler
    );

    if (hit)
    {
      add_possible_hit(out, normal);
    }
  }

  if (closest_hit != FLT_MAX)
  {
    out_coeff  = closest_hit;
    out_normal = hit_normal;
    return true;
  }
  else
  {
    return false;
  }
}

}

namespace nc
{

//==============================================================================
bool Frustum2::contains_point(vec2 p) const
{
  NC_ASSERT(is_normal(this->direction));

  if (is_full()) [[unlikely]]
  {
    return true;
  }

  const auto to_point = p - center;

  if (length(to_point) == 0.0f)
  {
    return true;
  }

  const auto projected = dot(normalize(to_point), direction);

  // Use > instead od >= for cases when the frustum is empty
  return projected > this->angle;
}

//==============================================================================
bool Frustum2::intersects_segment(vec2 p1, vec2 p2) const
{
  NC_ASSERT(is_normal(this->direction));

  if (this->is_empty()) [[unlikely]]
  {
    return false;
  }

  if (this->is_full()) [[unlikely]]
  {
    return true;
  }

  if (this->contains_point(p1) || this->contains_point(p2))
  {
    return true;
  }

  const auto to_p1 = normalize(p1 - this->center);
  const auto to_p2 = normalize(p2 - this->center);
  const auto sgn1 = sgn(cross(to_p1, this->direction));
  const auto sgn2 = sgn(cross(to_p2, this->direction));

  if (sgn1 == sgn2)
  {
    // both points of the wall are on the same side,
    // therefore the wall does not intersect with
    // the frustum
    return false;
  }

  // intersect the direction and wall
  const auto p1_to_p2 = p2 - p1;
  const auto center_to_p1 = p1 - this->center;
  const auto top = cross(p1_to_p2, center_to_p1);
  const auto bot = cross(p1_to_p2, this->direction);

  // should never happen as both points are on different sides
  NC_ASSERT(bot != 0.0f);

  // if t is 0 or more then the wall intersects with the direction
  // line and therefore is contained in the frustum
  const auto t = top / bot;
  return t >= 0.0f;
}

//==============================================================================
bool Frustum2::is_full() const
{
  return this->angle <= FULL_ANGLE;
}

//==============================================================================
bool Frustum2::is_empty() const
{
  return this->angle >= EMPTY_ANGLE;
}

//==============================================================================
static auto get_edge(vec2 dir, f32 dot_angle, bool left)
{
  NC_ASSERT(is_normal(dir));

  const auto flp = flipped(dir);
  const auto a = dot_angle;
  const auto b = std::sqrt(1.0f - a * a);

  const auto sign = left ? 1.0f : -1.0f;
  const auto edge = dir * a + flp * b * sign;
  NC_ASSERT(is_normal(edge));

  return edge;
}

//==============================================================================
static auto get_edges(const Frustum2& f)
{
  struct Edges
  {
    vec2 e1;
    vec2 e2;
  };

  const auto l_edge = get_edge(f.direction, f.angle, true);
  const auto r_edge = get_edge(f.direction, f.angle, false);

  // And these should hold as well
  NC_ASSERT(is_zero(dot(l_edge, f.direction) - f.angle, 0.0001f));
  NC_ASSERT(is_zero(dot(r_edge, f.direction) - f.angle, 0.0001f));

  return Edges{ .e1 = l_edge, .e2 = r_edge };
}

//==============================================================================
Frustum2 Frustum2::modified_with_portal(vec2 p1, vec2 p2) const
{
  NC_ASSERT(is_normal(this->direction));

  // Is this necessary?
  NC_ASSERT(p1 != this->center);
  NC_ASSERT(p2 != this->center);
  NC_ASSERT(p1 != p2);

  // this is unlikely as only the start frustum is (sometimes) full
  // and all others are not
  if (this->is_full()) [[unlikely]]
  {
    return Frustum2::from_point_and_portal(this->center, p1, p2);
  }

  // val is from 1 to -1
  // 1  = in front of us
  // -1 = right behind us
  // 0  = left/right from us 90deg
  // remaps to range [-1, 1] where -1 is fully left, 1 is fully right and 0 is in front of us
  auto remap_interval = [](f32 val, bool on_left)->f32
  {
    val = std::clamp<f32>(val, -1, 1);
    const f32 sign = on_left ? -1.0f : 1.0f;
    return (1.0f - (val + 1.0f) * 0.5f) * sign;
  };

  auto inverse_remap = [](f32 val)
  {
    NC_ASSERT(val == std::clamp<f32>(val, -1, 1));

    const bool on_left = val < 0.0f;
    const auto remapped = (1.0f - std::abs(val)) * 2.0f - 1.0f;

    return std::make_pair(on_left, remapped);
  };

  // interval 1
  const f32 e_left = remap_interval(this->angle, true);
  const f32 e_right = remap_interval(this->angle, false);
  NC_ASSERT(e_left <= e_right);

  // interval 2
  auto dt_p1 = dot(normalize(p1 - this->center), this->direction);
  auto dt_p2 = dot(normalize(p2 - this->center), this->direction);

  const auto p1_to_p2 = normalize(p2 - p1);
  const auto p2_to_p1 = -p1_to_p2;

  if (dt_p1 < 0.0f)
  {
    // it is in back
    const auto p2_to_center = this->center - p2;
    p1 = p2 + p2_to_p1 * dot(p2_to_center, p2_to_p1);
    dt_p1 = dot(normalize(p1 - this->center), this->direction);
  }

  if (dt_p2 < 0.0f)
  {
    // it is in back
    const auto p1_to_center = this->center - p1;
    p2 = p1 + p1_to_p2 * dot(p1_to_center, p1_to_p2);
    dt_p2 = dot(normalize(p2 - this->center), this->direction);
  }

  const auto to_p1 = p1 - this->center;
  const auto to_p2 = p2 - this->center;

  // if the point is right in front of us then we treat it as if right
  const auto p1_left = cross(this->direction, to_p1) >= 0;
  const auto p2_left = cross(this->direction, to_p2) >= 0;

  const f32 e_p1 = remap_interval(dt_p1, p1_left);
  const f32 e_p2 = remap_interval(dt_p2, p2_left);

  const auto i1_l = e_left;
  const auto i1_r = e_right;
  const auto i2_l = std::min(e_p1, e_p2);
  const auto i2_r = std::max(e_p1, e_p2);

  const auto overlap_l = std::max(i1_l, i2_l);
  const auto overlap_r = std::min(i1_r, i2_r);

  // this is unlikely as we probably will not get here due to some
  // other check in the map system BFS code
  if (overlap_l > overlap_r) [[unlikely]]
  {
    // intervals do not intersect and therefore the result is empty
    return Frustum2::empty_frustum_from_point(this->center);
  }

  // remap the intervals back onto the dot-space
  const auto [i1_left, i1_remap] = inverse_remap(overlap_l);
  const auto [i2_left, i2_remap] = inverse_remap(overlap_r);

  // and calculate the directions
  const auto dir1 = get_edge(this->direction, i1_remap, i1_left);
  const auto dir2 = get_edge(this->direction, i2_remap, i2_left);

  // and now calculate the new frustum from the directions above
  const auto new_dir = normalize(dir1 + dir2); // the sum should not be a zero..
  const auto new_angle = dot(dir1, new_dir);

  // the new FOV should be smaller than the previous one
  //NC_ASSERT(new_angle >= this->angle);

  return Frustum2::from_point_angle_and_dir(this->center, new_dir, new_angle);
}

//==============================================================================
Frustum2 Frustum2::merged_with(const Frustum2& other) const
{
  NC_ASSERT(other.center == this->center);

  if (other.is_full() || this->is_full()) [[unlikely]]
  {
    return Frustum2
    {
      .center = this->center,
      .direction = this->direction,
      .angle = FULL_ANGLE,
    };
  }

  // Calculate left and right edges of the frustums and compare them
  // TODO: can be done in a faster way using SIMD by doing 2 sqrts for a price of 1
  const auto [this_l_edge, this_r_edge] = get_edges(*this);
  const auto [other_l_edge, other_r_edge] = get_edges(other);

  // Should be normal
  NC_ASSERT(is_normal(this_l_edge) && is_normal(this_r_edge));
  NC_ASSERT(is_normal(other_l_edge) && is_normal(other_r_edge));

  const f32 lr = dot(this_l_edge, other_r_edge);
  const f32 rl = dot(this_r_edge, other_l_edge);

  // Now check the smaller one
  const bool lr_smaller = lr < rl;
  const auto l_edge = lr_smaller ? this_l_edge : other_l_edge;
  const auto r_edge = lr_smaller ? other_r_edge : this_r_edge;

  // negative if turned backwards, will help us flip the dir
  //const auto smaller_sgn = sgn(std::min(lr, rl)); 

  // Good, then calc the dir
  auto new_direction = normalize((l_edge + r_edge) /** smaller_sgn*/);

  if (is_zero(new_direction)) [[unlikely]]
  {
    // This happens if l_edge and r_edge point in an opposite directions.
    // We flip one of them around in an appropriate direction
    const auto l_edge2 = !lr_smaller ? this_l_edge : other_l_edge;
    const auto r_edge2 = !lr_smaller ? other_r_edge : this_r_edge;
    const auto greater_sgn = sgn(std::max(lr, rl));
    const auto new_direction2 = (l_edge2 + r_edge2) * greater_sgn;

    // This could theoretically happen if both angles are 0
    NC_ASSERT(!is_zero(new_direction2));

    new_direction = flipped(r_edge);
    // The dot product will end up being negative if the flipped vector points in a wrong direction
    new_direction = new_direction * sgn(dot(new_direction, new_direction2));
  }

  const auto new_angle = dot(l_edge, new_direction);
  NC_ASSERT(new_angle >= 0.0f);

  return Frustum2
  {
    .center = this->center,
    .direction = new_direction,
    .angle = new_angle,
  };
}

//==============================================================================
void Frustum2::get_frustum_edges(vec2& left, vec2& right) const
{
  const auto [l, r] = get_edges(*this);
  left = l;
  right = r;
}

//==============================================================================
f32 Frustum2::angle_difference(const Frustum2& other) const
{
  NC_ASSERT(other.center == this->center);

  // First, calculate the angle difference between directions.
  // We can do this by using dot product and asin.
  // TODO: Maybe do this without the asins?
  const f32 diff_of_directions = std::acos(dot(this->direction, other.direction));
  const f32 deg1 = std::acos(this->angle);
  const f32 deg2 = std::acos(other.angle);

  return diff_of_directions - deg1 - deg2;
}

//==============================================================================
Frustum2 Frustum2::from_point_and_portal(vec2 point, vec2 a, vec2 b)
{
  auto new_frustum = Frustum2
  {
    .center = point,
    .direction = vec2{0, 1},
    .angle = FULL_ANGLE,
  };

  if (point == a || point == b)
  {
    return new_frustum;
  }

  const auto to_a = normalize(a - point);
  const auto to_b = normalize(b - point);

  if (dot(to_a, to_b) == -1.0f)
  {
    // stuck in the wall, return empty frustum
    new_frustum.angle = 1.0f;
    new_frustum.direction = vec2{ to_a.y, to_a.x };

    NC_ASSERT(new_frustum.is_empty());

    return new_frustum;
  }

  new_frustum.direction = normalize(to_a + to_b);
  new_frustum.angle = dot(to_a, new_frustum.direction);
  NC_ASSERT(new_frustum.angle >= 0.0f);
  return new_frustum;
}

//==============================================================================
Frustum2 Frustum2::empty_frustum_from_point(vec2 c)
{
  return Frustum2
  {
    .center = c,
    .direction = vec2{1, 0},
    .angle = EMPTY_ANGLE,
  };
}

//==============================================================================
Frustum2 Frustum2::from_point_angle_and_dir(vec2 point, vec2 dir, f32 angle)
{
  NC_ASSERT(is_normal(dir));
  //NC_ASSERT(angle == std::clamp<f32>(angle, -1, 1));

  return Frustum2
  {
    .center = point,
    .direction = dir,
    .angle = angle,
  };
}

//==============================================================================
void FrustumBuffer::insert_frustum(Frustum2 new_frustum)
{
  u64  closest_idx = 0;
  f32  closest_dst = FLT_MAX;

  for (u64 i = 0; i < FRUSTUM_SLOT_CNT; ++i)
  {
    auto& other_frustum = this->frustum_slots[i];
    auto  is_invalid = other_frustum == INVALID_FRUSTUM;

    f32 angle_diff = is_invalid ? 0.0f : other_frustum.angle_difference(new_frustum);

    // 3 degrees of threshold due to numerical instability
    static const f32 threshold = deg2rad(3.0f);

    if (angle_diff <= threshold)
    {
      closest_idx = i;
      break;
    }

    // not overlapping, but might be quite close
    if (angle_diff < closest_dst)
    {
      angle_diff = closest_dst;
      closest_idx = i;
    }
  }

  NC_ASSERT(closest_idx < FRUSTUM_SLOT_CNT);

  // No overlapping frustum found and all slots are full?
  // Then merge with a closest one
  auto& closest_frustum = this->frustum_slots[closest_idx];
  if (closest_frustum == INVALID_FRUSTUM)
  {
    closest_frustum = new_frustum;
  }
  else
  {
    closest_frustum = closest_frustum.merged_with(new_frustum);
  }
}

}
