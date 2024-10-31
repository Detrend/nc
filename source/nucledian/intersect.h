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
  vec2 center    = vec2{0};     // a center point of the frustum
  vec2 direction = vec2{0, 1};  // a direction in which it is facing
  // angle in [-1.0, 1.0] interval
  // -1 = 360 deg frustum, 1 = 0 degree frustum
  f32  angle = -2.0f;

  bool contains_point(vec2 point)        const;
  bool intersects_wall(vec2 p1, vec2 p2) const;

  bool is_full()  const;
  bool is_empty() const;

  // inserts a "portal" into the frustum, modifying it
  Frustum2 modify_with_wall(vec2 p1, vec2 p2) const;
};

}

namespace nc::intersect
{

bool segment_segment(vec2 start_a, vec2 end_a, vec2 start_b, vec2 end_b);

bool aabb_aabb_2d(const aabb2& a, const aabb2& b);

bool point_triangle(vec2 point, vec2 a, vec2 b, vec2 c);

bool triangle_triangle(vec2 a1, vec2 b1, vec2 c1, vec2 a2, vec2 b2, vec2 c2);

bool convex_convex(std::span<vec2> a, std::span<vec2> b, f32 threshold = 0.01f);

}

namespace nc::intersect::sse
{

bool point_triangle(vec2 point, vec2 a, vec2 b, vec2 c);

}

