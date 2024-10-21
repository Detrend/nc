// Project Nucledian Source File
#include <intersect.h>
#include <vector_maths.h>
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

