// Project Nucledian Source File
#pragma once

#include <types.h>
#include <vector_maths.h>
#include <aabb.h>

#include <span>

namespace nc
{

// A convex 2D view frustum. Has a position and view angle.
// Can perform basic operations like checking if an object is inside
// or outside or shrinking the frustum.
// Maximal supported angle is 180 degrees. All other angles are threaded
// as 360 degree.
struct Frustum2
{
  static constexpr f32 EMPTY_ANGLE = 1.0f;
  static constexpr f32 FULL_ANGLE  = 0.0f;

  vec2 center    = vec2{0};     // a center point of the frustum
  vec2 direction = vec2{0, 1};  // a direction in which it is facing, has to be normalized
  // angle in [-1.0, 1.0] interval
  // -1 = 360 deg frustum, 1 = 0 degree frustum
  f32  angle = FULL_ANGLE;

  // Defaulted comparison so we can compare with INVALID_FRUSTUM
  bool operator==(const Frustum2&) const = default;

  // Checks if a given point is inside the frustum
  bool contains_point(vec2 point)           const;
  // Checks if the line segment is partially or fully inside the frustum
  bool intersects_segment(vec2 p1, vec2 p2) const;

  // Checks if the frustum angle is 360 degrees
  bool is_full()  const;
  // Checks if the frustum angle is 0 degrees
  bool is_empty() const;

  // Returns a frustum modified by this portal
  Frustum2 modied_with_portal(vec2 p1, vec2 p2) const;

  // Merges the two frustums together into one and returns it
  Frustum2 merged_with(const Frustum2& other) const;

  // Calculates the difference between two frustums in radians.
  // Negative if frustums overlap
  f32 angle_difference(const Frustum2& other) const;

  // Constructs a new frustum from center point and two portal points
  static Frustum2 from_point_and_portal(vec2 point, vec2 a, vec2 b);
};

constexpr Frustum2 INVALID_FRUSTUM = Frustum2{vec2{0}, vec2{0}, Frustum2::EMPTY_ANGLE};

}

// Intersection tests are a set of tests that usually only answer true/false
// if two primitives intersect or not.
// This is always faster or at least the same as testing for distance.
// If you want more information about position of two primitives then
// use set of distance tests or casting tests.
namespace nc::intersect
{
// Checks for an intersection of two 2D line segments and returns true if
// they intersect.
bool segment_segment(vec2 start_a, vec2 end_a, vec2 start_b, vec2 end_b);

// Checks for intersection of two 2D AABBs and returns true if they intersect.
bool aabb_aabb_2d(const aabb2& a, const aabb2& b);

// Checks for intersection of 2D point with 2D triangle.
// True is returned if the point lies in the triangle or on one of
// its edges.
bool point_triangle(vec2 point, vec2 a, vec2 b, vec2 c);

// Checks for intersection of two convex primitives specified as a set of 2D
// points. Tries finding an ideal split plane and performs asymptotically in
// O(n+m) where n is number of points in the first primitive and m in the
// second one.
// The last parameter is a threshold distance, leave default if you do not
// know what you are doing.
// TODO: do we really need a threshold?
bool convex_convex(std::span<vec2> a, std::span<vec2> b, f32 threshold = 0.01f);

// Checks for intersection of two 2D triangles using the same method as
// convex-to-convex test.
bool triangle_triangle(vec2 a1, vec2 b1, vec2 c1, vec2 a2, vec2 b2, vec2 c2);

}

