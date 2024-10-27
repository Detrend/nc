// Project Nucledian Source File
#include <intersect.h>
#include <vector_maths.h>
#include <aabb.h>

#include <limits>     // FLT_MIN, FLT_MAX
#include <algorithm>  // std::min, std::max
#include <array>

namespace nc::intersect
{
  
//==============================================================================
bool segment_segment_2d(
  vec2  start_a,
  vec2  end_a,
  vec2  start_b,
  vec2  end_b,
  f32&  t,
  bool& parallel)
{
  vec2 dir_a = end_a - start_a;
  vec2 dir_b = end_b - start_b;
  f32 top    = cross(dir_a, dir_b);
  f32 bottom = cross(start_b - start_a, dir_b);

  if (top == 0 && bottom == 0)
  {
    parallel = true;
    return true;
  }
  else if (top == 0)
  {
    // parallel and non-intersecting
    parallel = true;
    return false;
  }
  else
  {
    parallel = false;

    t = top / bottom;
    if (t >= 0 && t <= 1)
    {
      // might be intersecting, check for u as well
      return true;
    }

    // intersecting, but only lines, not segments
  }

  return false;   // TODO: remove
}

//==============================================================================
bool aabb_aabb_2d(const aabb2& a, const aabb2& b)
{
  vec2 mn = max(a.min, b.min);
  vec2 mx = min(a.max, b.max);
  aabb2 new_aabb{mn, mx};
  return new_aabb.is_valid();
}

//==============================================================================
static f32 sign(f32 input)
{
  return input == 0.0f ? 0.0f : (input > 0.0f ? 1.0f : -1.0f);
}

//==============================================================================
bool point_triangle(vec2 p, vec2 a, vec2 b, vec2 c)
{
  const auto a_to_b = b-a;
  const auto b_to_c = c-b;
  const auto c_to_a = a-c;

  const auto a_to_p = p-a;
  const auto b_to_p = p-b;
  const auto c_to_p = p-c;

  const f32 gl_sign = sign(cross(a_to_b, b_to_c));

  const f32 a_sign = sign(cross(a_to_b, a_to_p));
  const f32 b_sign = sign(cross(b_to_c, b_to_p));
  const f32 c_sign = sign(cross(c_to_a, c_to_p));

  auto eq_or_zero = [](f32 f1, f32 f2)
  {
    return f1 == f2 || f2 == 0.0f;
  };

  return eq_or_zero(gl_sign, a_sign)
      && eq_or_zero(gl_sign, b_sign)
      && eq_or_zero(gl_sign, c_sign);
}

//==============================================================================
bool sse::point_triangle(vec2 p, vec2 a, vec2 b, vec2 c)
{
  const auto a_to_b = b-a;
  const auto b_to_c = c-b;
  const auto c_to_a = a-c;

  const auto a_to_p = p-a;
  const auto b_to_p = p-b;
  const auto c_to_p = p-c;

  const f32 g_sign = sign(cross(a_to_b, b_to_c));
  const f32 a_sign = sign(cross(a_to_b, a_to_p));
  const f32 b_sign = sign(cross(b_to_c, b_to_p));
  const f32 c_sign = sign(cross(c_to_a, c_to_p));

  auto eq_or_zero = [](f32 f1, f32 f2)
  {
    return f1 == f2 || f2 == 0.0f;
  };

  return eq_or_zero(g_sign, a_sign)
      && eq_or_zero(g_sign, b_sign)
      && eq_or_zero(g_sign, c_sign);
}

//==============================================================================
bool triangle_triangle(vec2 a1, vec2 b1, vec2 c1, vec2 a2, vec2 b2, vec2 c2)
{
  auto arr1 = std::array{a1, b1, c1};
  auto arr2 = std::array{a2, b2, c2};
  return convex_convex(
    std::span{arr1.data(), arr1.size()},
    std::span{arr2.data(), arr2.size()});
}

//==============================================================================
bool convex_convex(std::span<vec2> a, std::span<vec2> b, f32 threshold)
{
  // iterate edges of both shapes and search for a line that separates
  // both of them
  const std::span<vec2> points[2] = {a, b};

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
      const auto  line   = normalize(p1-p2);
      const auto  normal = vec2{line.y, -line.x};

      struct LocalInterval
      {
        f32 min = FLT_MAX;
        f32 max = FLT_MIN;
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
    }
  }

  // there is no hyperplane that separates the two shapes, therefore
  // they must intersect
  return true;
}

}

//==============================================================================
bool nc::Frustum2::contains_point(vec2 /*p*/) const
{
  return false;
}

//==============================================================================
bool nc::Frustum2::intersects_wall(vec2 /*p1*/, vec2 /*p2*/) const
{
  return false;
}

//==============================================================================
nc::Frustum2 nc::Frustum2::modify_with_wall(vec2 /*p1*/, vec2 /*p2*/) const
{
  return Frustum2{};
}

