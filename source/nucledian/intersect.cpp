// Project Nucledian Source File
#include <config.h>

#include <intersect.h>
#include <common.h>

#include <aabb.h>

#include <math/utils.h>
#include <math/vector.h>
#include <math/lingebra.h>

#ifdef NC_TESTS
#include <unit_test.h>
#endif

#include <limits>     // FLT_MAX
#include <algorithm>  // std::min, std::max, std::abs
#include <array>
#include <cmath>

namespace nc::intersect
{

//==============================================================================
bool ray_segment
(
  vec2 start_a,
  vec2 end_a,
  vec2 start_b,
  vec2 end_b,
  f32& t_out,
  f32& u_out
)
{
  // This wouldn't be a problem, but lets just for safety check the args
  nc_assert(start_a != end_a);
  nc_assert(start_b != end_b);

  t_out = FLT_MAX;
  u_out = FLT_MAX;

  const vec2 dir_a  = end_a - start_a;
  const vec2 dir_b  = end_b - start_b;
  const f32  top    = cross(dir_b, start_b - start_a);
  const f32  bottom = cross(dir_b, dir_a);

  if (top == 0.0f && bottom == 0.0f)
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
  else if (bottom == 0.0f)
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
    nc_assert(ubottom != 0.0f);

    const f32 u = utop / ubottom;

    t_out = t;
    u_out = u;

    // lines intersect, but segments do not
    return u >= 0.0f && u <= 1.0f;
  }
}

//==============================================================================
bool segment_segment
(
  vec2 start_a,
  vec2 end_a,
  vec2 start_b,
  vec2 end_b,
  f32& t_out,
  f32& u_out
)
{
  // This wouldn't be a problem, but lets just for safety check the args
  nc_assert(start_a != end_a);
  nc_assert(start_b != end_b);

  t_out = FLT_MAX;
  u_out = FLT_MAX;

  const vec2 dir_a  = end_a - start_a;
  const vec2 dir_b  = end_b - start_b;
  const f32  top    = cross(dir_b, start_b - start_a);
  const f32  bottom = cross(dir_b, dir_a);

  if (top == 0.0f && bottom == 0.0f)
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
  else if (bottom == 0.0f)
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
    nc_assert(ubottom != 0.0f);

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
bool segment_circle
(
  vec2  ray_start,
  vec2  ray_end,
  vec2  circle_center,
  f32   circle_radius,
  f32&  t_out,
  vec2& n_out
)
{
  nc_assert(ray_start != ray_end);
  nc_assert(circle_radius > 0.0f);

  t_out = FLT_MAX;
  n_out = VEC2_ZERO;

  // First check if we are not pointing away.. In such a case do not collide
  // at all
  const vec2 center_to_start = ray_start - circle_center;
  const vec2 ray_dir         = ray_end - ray_start;
  if (dot(ray_dir, center_to_start) >= 0.0f)
  {
    // We are heading in the direction away from the circle
    return false;
  }

  // Then handle the case when the ray starts inside the circle
  const f32 distance_to_center = length(center_to_start);
  if (distance_to_center < circle_radius)
  {
    const f32 dist_to_r  = circle_radius - distance_to_center;
    const f32 ray_length = length(ray_end - ray_start);

    t_out = std::min(dist_to_r / ray_length, 1.0f) - 1.0f;
    n_out = is_zero(center_to_start) ? vec2{1, 0} : normalize(center_to_start);

    return true;
  }

  const vec2 center = circle_center - ray_start;

  const f32 dx = ray_dir.x;
  const f32 dy = ray_dir.y;
  const f32 cx = center.x;
  const f32 cy = center.y;
  const f32 rr = circle_radius * circle_radius;

  const f32 a = dx * dx + dy * dy;
  const f32 b = -2 * (dx * cx + dy * cy);
  const f32 c = -1 * (rr - cx * cx - cy * cy);
  nc_assert(a != 0.0f);

  const f32 D2 = b * b - 4 * a * c;
  if (D2 < 0.0f)
  {
    // no solution
    return false;
  }

  const f32 a2 = 2 * a;
  const f32 D  = std::sqrtf(D2);

  const f32 t1 = (-b + D) / a2;
  const f32 t2 = (-b - D) / a2;

  const bool t1_legit = t1 == std::clamp(t1, 0.0f, 1.0f);
  const bool t2_legit = t2 == std::clamp(t2, 0.0f, 1.0f);

  f32 t = std::min(t1_legit ? t1 : FLT_MAX, t2_legit ? t2 : FLT_MAX);

  if (t != FLT_MAX)
  {
    const vec2 col_point = ray_dir * t;
    nc_assert(col_point != center);

    const vec2 center_to_col = col_point - center;

    t_out = t;
    n_out = normalize(center_to_col);
    return true;
  }
  else
  {
    return false;
  }
}

//==============================================================================
bool ray_wall_3d
(
  vec3  ray_start,
  vec3  ray_end,
  vec2  wall_a,
  vec2  wall_b,
  f32   wall_y1,
  f32   wall_y2,
  f32&  out_coeff
)
{
  nc_assert(wall_y1   != wall_y2, "Points of the wall are the same.");
  nc_assert(ray_start != ray_end, "Ray has a zero length.");

  out_coeff = FLT_MAX;

  const vec2 start2d = ray_start.xz;
  const vec2 end2d   = ray_end.xz;
  const vec3 ray_dir = ray_end - ray_start;

  if (f32 _; !segment_segment(start2d, end2d, wall_a, wall_b, out_coeff, _))
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
  nc_assert(ray_start != ray_end);
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
  nc_assert(is_normal(this->direction));

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
  nc_assert(is_normal(this->direction));

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
  nc_assert(bot != 0.0f);

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
  nc_assert(is_normal(dir));

  const auto flp = flipped(dir);
  const auto a = dot_angle;
  const auto b = std::sqrt(1.0f - a * a);

  const auto sign = left ? 1.0f : -1.0f;
  const auto edge = dir * a + flp * b * sign;
  nc_assert(is_normal(edge));

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
  nc_assert(is_zero(dot(l_edge, f.direction) - f.angle, 0.0001f));
  nc_assert(is_zero(dot(r_edge, f.direction) - f.angle, 0.0001f));

  return Edges{ .e1 = l_edge, .e2 = r_edge };
}

//==============================================================================
static bool interval_intersection
(
  f32 a_l, f32 a_r, f32 b_l, f32 b_r, f32& out_l, f32& out_r
)
{
  // We assume that all of them are in [0, PI] interval
  nc_assert(a_l >= 0.0f && a_l <= PI2);
  nc_assert(a_r >= 0.0f && a_r <= PI2);
  nc_assert(b_l >= 0.0f && b_l <= PI2);
  nc_assert(b_r >= 0.0f && b_r <= PI2);

  // Reset
  out_l = 0.0f;
  out_r = 0.0f;

  // 4 cases
  bool a_split = a_r > a_l;
  bool b_split = b_r > b_l;

  if (!a_split && !b_split)
  {
    // ezz
    out_l = std::min(a_l, b_l);
    out_r = std::max(a_r, b_r);
    return out_l > out_r;
  }
  else if (a_split && b_split)
  {
    // must have intersection at 0
    out_l = std::min(a_l, b_l);
    out_r = std::max(a_r, b_r);
    return out_l != 0.0f && out_r != PI2;
  }
  else
  {
    if (b_split)
    {
      // swap the intervals so that a is always the split one
      std::swap(a_l, b_l);
      std::swap(a_r, b_r);
    }

    // now B will be either fully on one side or on another, but never
    // in between
    if (b_r < a_l)
    {
      out_l = b_r;
      out_r = a_l;
      return true;
    }
    else if (b_l > a_r)
    {
      out_l = a_r;
      out_r = b_l;
      return true;
    }
    else
    {
      return false;
    }
  }
}

//==============================================================================
f32 to_0_pi(f32 in)
{
  if (in < 0.0f)
  {
    return in + PI2;
  }
  else
  {
    return in;
  }
}

//==============================================================================
// 0    = right
// PI/2 = up
// PI   = left
static vec2 dir_from_angle(f32 angle)
{
  f32 x = std::cos(angle);
  f32 y = std::sin(angle);
  return vec2{x, y};
}

//==============================================================================
Frustum2 Frustum2::modified_with_portal(vec2 p1, vec2 p2) const
{
  nc_assert(is_normal(this->direction));

  // Is this necessary?
  nc_assert(p1 != this->center);
  nc_assert(p2 != this->center);
  nc_assert(p1 != p2);

  // this is unlikely as only the start frustum is (sometimes) full
  // and all others are not
  if (this->is_full()) [[unlikely]]
  {
    return Frustum2::from_point_and_portal(this->center, p1, p2);
  }

  vec2 d_l, d_r;
  this->get_frustum_edges(d_l, d_r);

  vec2 to_p1 = p1 - this->center;
  vec2 to_p2 = p2 - this->center;

  f32 a_l = to_0_pi(std::atan2f(d_l.y,   d_l.x));
  f32 a_r = to_0_pi(std::atan2f(d_r.y,   d_r.x));
  f32 b_l = to_0_pi(std::atan2f(to_p2.y, to_p2.x));
  f32 b_r = to_0_pi(std::atan2f(to_p1.y, to_p1.x));

  if (f32 i1 = 0, i2 = 0; interval_intersection(a_l, a_r, b_l, b_r, i1, i2))
  {
    vec2 d1 = dir_from_angle(i1);
    vec2 d2 = dir_from_angle(i2);
    nc_assert(d1 + d2 != VEC2_ZERO);

    vec2 new_dir   = normalize(d1 + d2);
    f32  new_angle = dot(d1, new_dir);
    return Frustum2::from_point_angle_and_dir(this->center, new_dir, new_angle);
  }
  else
  {
    return Frustum2::empty_frustum_from_point(this->center);
  }
}

//==============================================================================
Frustum2 Frustum2::merged_with(const Frustum2& other) const
{
  // nc_assert(other.center == this->center);

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
  nc_assert(is_normal(this_l_edge) && is_normal(this_r_edge));
  nc_assert(is_normal(other_l_edge) && is_normal(other_r_edge));

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
    nc_assert(!is_zero(new_direction2));

    new_direction = flipped(r_edge);
    // The dot product will end up being negative if the flipped vector points in a wrong direction
    new_direction = new_direction * sgn(dot(new_direction, new_direction2));
  }

  const auto new_angle = dot(l_edge, new_direction);
  nc_assert(new_angle >= 0.0f);

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
  // nc_assert(other.center == this->center);

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

    nc_assert(new_frustum.is_empty());

    return new_frustum;
  }

  new_frustum.direction = normalize(to_a + to_b);
  new_frustum.angle = dot(to_a, new_frustum.direction);
  nc_assert(new_frustum.angle >= 0.0f);
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
  nc_assert(is_normal(dir));
  //nc_assert(angle == std::clamp<f32>(angle, -1, 1));

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

  nc_assert(closest_idx < FRUSTUM_SLOT_CNT);

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

namespace nc::collide
{
  
//==============================================================================
bool ray_exp_wall
(
  vec2  start_a,
  vec2  end_a,
  vec2  start_b,
  vec2  end_b,
  f32   wall_exp,
  vec2& out_normal,
  f32&  out_coeff
)
{
  using intersect::segment_circle;
  using intersect::ray_segment;

  nc_assert(start_a  != end_a);
  nc_assert(start_b  != end_b);
  nc_assert(wall_exp > 0.0f); // Use ray_wall for non-expanded collisions

  // Reset by default
  out_normal = VEC2_ZERO;
  out_coeff  = FLT_MAX;

  const vec2 ray_dir = end_a - start_a;

  const vec2 b_norm   = flipped(normalize(end_b - start_b));
  const vec2 b_offset = b_norm * wall_exp;

  bool ll_coll = false;
  bool cs_coll = false; 
  bool ce_coll = false; 

  vec2 cs_n, ce_n;
  f32 cs_coeff = 0, ce_coeff = 0, ll_coeff = 0;

  // Check the circles first
  cs_coll = segment_circle(start_a, end_a, start_b, wall_exp, cs_coeff, cs_n);
  ce_coll = segment_circle(start_a, end_a, end_b,   wall_exp, ce_coeff, ce_n);

  // And now check the wall itself. We only need to check the part of the wall facing us.
  // Also, we can (and should) do the checking only if the wall is in our direction
  // of movement. Otherwise, we could collide with it even if moving away from it
  // in the opposite direction. Thats what the dot(...) < 0 is for
  if (dot(ray_dir, b_norm) < 0.0f)
  {
    f32 _;
    ll_coll = ray_segment
    (
      start_a, end_a, start_b + b_offset, end_b + b_offset, ll_coeff, _
    );

    f32 max_coeff = 1.0f;
    f32 min_coeff = - wall_exp / length(ray_dir);
    //f32 min_coeff = 0;
    if (ll_coll && (ll_coeff < min_coeff || ll_coeff > max_coeff))
    {
      ll_coll = false;
    }
  }

  auto take_best = [&out_coeff, &out_normal]
  (
    bool intersection, f32 coeff, vec2 norm
  )
  {
    if (intersection && coeff < out_coeff)
    {
      out_coeff  = coeff;
      out_normal = norm;
    }
  };

  take_best(cs_coll, cs_coeff, cs_n);
  take_best(ce_coll, ce_coeff, ce_n);
  take_best(ll_coll, ll_coeff, b_norm);

  return out_coeff != FLT_MAX;
}

//==============================================================================
bool ray_wall
(
  vec2  start_a,
  vec2  end_a,
  vec2  start_b,
  vec2  end_b,
  vec2& out_normal,
  f32&  out_coeff
)
{
  f32 _;
  const bool intersects = intersect::segment_segment(start_a, end_a, start_b, end_b, out_coeff, _);

  const vec2 dir = end_a - start_a;

  out_normal = flipped(normalize(end_b - start_b));
  if (dot(dir, out_normal) > 0.0f)
  {
    out_normal = -out_normal;
  }

  return intersects && out_coeff >= 0.0f && out_coeff <= 1.0f;
}

//==============================================================================
bool ray_exp_cylinder
(
  vec2  ray_start,
  vec2  ray_end,
  vec2  center,
  f32   radius,
  f32   expansion,
  vec2& out_normal,
  f32&  out_coeff
)
{
  nc_assert(expansion >= 0.0f);
  nc_assert(radius >= 0.0f);

  out_normal = VEC2_ZERO;
  out_coeff  = FLT_MAX;

  if (ray_start == ray_end)
  {
    if (vec2 dir = center - ray_start; length(dir) <= expansion + radius)
    {
      out_coeff  = 0.0f;
      out_normal = dir == VEC2_ZERO ? VEC2_X : normalize(dir);
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return intersect::segment_circle
    (
      ray_start, ray_end, center, radius + expansion, out_coeff, out_normal
    );
  }
}

//==============================================================================
bool ray_plane_xz
(
  vec3  ray_start,
  vec3  ray_end,
  f32   plane_y,
  f32&  out_c
)
{
  // Check the ray dir
  vec3 ray_dir = ray_end - ray_start;

  // Early exit if the y is 0
  if (ray_dir.y == 0.0f)
  {
    return false;
  }

  // We know that there is a t such as
  // ray_start.y + ray_dir.y * t           = plane_y
  // ray_start.y - plane_y + ray_dir.y * t = 0.0f
  // ray_dir.y * t                         = plane_y - ray_start.y
  // t                                     = (plane_y - ray_start.y) / ray_dir.y
  out_c = (plane_y - ray_start.y) / ray_dir.y;
  return true;
}

}

namespace nc::dist
{

//==============================================================================
template<typename TVec>
f32 point_aabb(TVec pt, const aabb<f32, TVec::length()>& bbox)
{
  nc_assert(bbox.is_valid());
  TVec clamped_pt = clamp(pt, bbox.min, bbox.max);
  return length(clamped_pt - pt);
}

template f32 point_aabb<vec3>(vec3, const aabb3&);
template f32 point_aabb<vec2>(vec2, const aabb2&);

//==============================================================================
f32 point_line_2d(vec2 p, vec2 a, vec2 b)
{
  // Project on the line, clip and measure the distance
  f32  ab_len     = length(b - a);
  vec2 line_dir_n = normalize_or_zero(b - a);
  vec2 to_pt_dir  = p - a;
  f32  coeff      = dot(to_pt_dir, line_dir_n);
  f32  bounded_c  = std::clamp(coeff, 0.0f, ab_len);
  vec2 reproj     = line_dir_n * bounded_c + a;

  return distance(reproj, p);
}

//==============================================================================
f32 segment_segment_2d
(
  vec2 l1_a,
  vec2 l1_b,
  vec2 l2_a,
  vec2 l2_b
)
{
  if (f32 _, __; intersect::segment_segment(l1_a, l1_b, l2_a, l2_b, _, __))
  {
    return 0.0f;
  }

  f32 d1 = point_line_2d(l1_a, l2_a, l2_b);
  f32 d2 = point_line_2d(l1_b, l2_a, l2_b);
  f32 d3 = point_line_2d(l2_a, l1_a, l1_b);
  f32 d4 = point_line_2d(l2_b, l1_a, l1_b);

  return std::min({d1, d2, d3, d4});
}

}

#ifdef NC_TESTS
namespace nc
{

bool test_segment(unit_test::TestCtx& /*ctx*/)
{
  f32 t, u;
  intersect::segment_segment(vec2{-1, 0}, vec2{1, 0}, vec2{0, -1}, vec2{0, 1}, t, u);
  if (t == 0.5f && u == 0.5f)
  {
    NC_TEST_SUCCESS;
  }
  else
  {
    NC_TEST_FAIL;
  }
}
NC_UNIT_TEST(test_segment)->name("Segment segment intersection");

}
#endif
