// Project Nucledian Source File
#pragma once

#include <types.h>
#include <vector_maths.h>
#include <aabb.h>

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

}

namespace nc::intersect::sse
{

bool point_triangle(vec2 point, vec2 a, vec2 b, vec2 c);

}

