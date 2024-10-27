// Project Nucledian Source File
#pragma once

#include <types.h>
#include <vector_maths.h>
#include <aabb.h>

#include <span>

namespace nc
{

struct Frustum2
{
  vec2 point = vec2{0};
  vec2 dir1  = vec2{0};
  vec2 dir2  = vec2{0};

  bool contains_point(vec2 point)        const;
  bool intersects_wall(vec2 p1, vec2 p2) const;

  Frustum2 modify_with_wall(vec2 p1, vec2 p2) const;
};

}

namespace nc::intersect
{

bool segment_segment_2d(vec2 start_a, vec2 end_a, vec2 start_b, vec2 end_b, f32& t, bool& parallel);

bool aabb_aabb_2d(const aabb2& a, const aabb2& b);

bool point_triangle(vec2 point, vec2 a, vec2 b, vec2 c);

bool triangle_triangle(vec2 a1, vec2 b1, vec2 c1, vec2 a2, vec2 b2, vec2 c2);

bool convex_convex(std::span<vec2> a, std::span<vec2> b, f32 threshold = 0.01f);

}

namespace nc::intersect::sse
{

bool point_triangle(vec2 point, vec2 a, vec2 b, vec2 c);

}

