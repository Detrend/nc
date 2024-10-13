// Project Nucledian Source File
#pragma once

#include <types.h>
#include <vector_maths.h>

namespace nc::intersect
{

bool segment_segment_2d(vec2 start_a, vec2 end_a, vec2 start_b, vec2 end_b, vec2* intersection, bool* parallel)
{
  vec2 dir_a = end_a - start_a;
  vec2 dir_b = end_b - start_b;
  f32 top    = cross(dir_a, dir_b);
  f32 bottom = cross(start_b - start_a, dir_b);

  if (top == 0 && bottom == 0)
  {
    // lines are parallel and intersecting
    if (parallel)
    {
      *parallel = true;
      return true;
    }
  }
  else if (top == 0)
  {
    // parallel and non-intersecting
    if (parallel)
    {
      *parallel = true;
    }
    return false;
  }
  else
  {
    f32 t = top / bottom;
    if (t >= 0 && t <= 1)
    {
      // intersecting
      return true;
    }

    // intersecting, but only lines, not segments
  }
}

bool aabb_aabb_2d(const aabb2& a, const aabb2& b)
{
  vec2 mn = max(a.min, b.min);
  vec2 mx = min(a.max, b.max);
  aabb2 new_aabb{mn, mx};
  return new_aabb.is_valid();
}

}

