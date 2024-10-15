// Project Nucledian Source File
#include <intersect.h>
#include <aabb.h>

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

}

