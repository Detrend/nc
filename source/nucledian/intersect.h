// Project Nucledian Source File
#pragma once

#include <types.h>
#include <math/vector.h>
#include <aabb.h>

#include <span>
#include <array>

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
  Frustum2 modified_with_portal(vec2 p1, vec2 p2) const;

  // Merges the two frustums together into one and returns it
  Frustum2 merged_with(const Frustum2& other) const;

  void get_frustum_edges(vec2& left, vec2& right) const;

  // Calculates the difference between two frustums in radians.
  // Negative if frustums overlap
  f32 angle_difference(const Frustum2& other) const;

  // Constructs a new frustum from center point and two portal points
  static Frustum2 from_point_and_portal(vec2 point, vec2 a, vec2 b);

  static Frustum2 empty_frustum_from_point(vec2 center);

  static Frustum2 from_point_angle_and_dir(vec2 point, vec2 dir, f32 angle);
};
constexpr Frustum2 INVALID_FRUSTUM = Frustum2{vec2{0}, vec2{0}, Frustum2::EMPTY_ANGLE};

// A set of frustums
struct FrustumBuffer
{
	static constexpr u64 FRUSTUM_SLOT_CNT = 4;
  using FrustumArray = std::array<Frustum2, FRUSTUM_SLOT_CNT>;

  FrustumArray frustum_slots;

  explicit FrustumBuffer(Frustum2 from_frustum)
  {
    frustum_slots.fill(INVALID_FRUSTUM);
    frustum_slots[0] = from_frustum;
  }

  FrustumBuffer() : FrustumBuffer(INVALID_FRUSTUM){};

  // This merges a new frustum with overlapping one. If no overlapping frustum
  // is found then inserts it or merges with a closest one.
  void insert_frustum(Frustum2 new_frustum);
};

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
// Intersection points of both segments can be calculated from t_out and
// u_out parameters.
// If the function returns true but both u_out and t_out are FLT_MAX then
// the segments are parallel and intersecting.
bool segment_segment(
  vec2 start_a,
  vec2 end_a,
  vec2 start_b,
  vec2 end_b,
  f32& t_out,
  f32& u_out);

// Returns true if the infinite ray collides with a finite wall segment.
// If so, then the ray_c_out contains coeff for intersection pt reconstruction
bool ray_segment(
  vec2 ray_start,
  vec2 ray_end,
  vec2 seg_a,
  vec2 seg_b,
  f32& ray_c_out);

// Performs a segment-circle intersection.
// Returns true if the segment and the circle intersect.
bool segment_circle(vec2 start, vec2 end, vec2 og_center, f32 r, f32& t_out, vec2& n_out);

bool ray_wall_3d(
  vec3 ray_start,
  vec3 ray_end,
  vec2 wall_a,
  vec2 wall_b,
  f32  wall_y1,
  f32  wall_y2,
  f32& out_coeff);

// Checks for intersection between segmented ray (has start and end) and infinite
// horizontal plane with normal vector (0, 1, 0) and height plane_y.
// Returns true if the ray segment intersects the plane and outputs the coefficient
// of intersection as "out_coeff". The point of intersection can be calculated as
// ray_start + (ray_end - ray_start) * out_coeff.
// The "out_coeff" value is invalid (and possibly non-inicialized) if the function
// returns false and therefore it should not be used in any further computations.
bool ray_infinite_plane_xz(
  vec3 ray_start,
  vec3 ray_end,
  f32  plane_y,
  f32& out_coeff);

// Checks for intersection of ray segment and 3d bounding box. Returns true if the
// two intersect. If so, then also fills out the "out_normal" with normal vector
// of the intersecting point and "out_coeff" with coefficient of the intersection,
// from which the intersection point can be calculated as follows:
// hit_point = ray_start + (ray_end - ray_start) * out_coeff;
bool ray_aabb3(
  vec3  ray_start,
  vec3  ray_end,
  aabb3 bbox,
  f32&  out_coeff,
  vec3& out_normal);

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

namespace nc::collide
{
  
// Checks for intersection between two segments, second of which is
// expanded by a value.
// Returns true if the two segments intersect. In such case the normal
// vector of the intersection and a point of intersection are returned.
// Point of intersection can be calculated as:
// vec2 intersect_pt = start_a + (end_a - start_a) * out_coeff;
bool ray_exp_wall(
  vec2  ray_start,
  vec2  ray_end,
  vec2  wall_start,
  vec2  wall_end,
  f32   wall_exp,
  vec2& out_normal,
  f32&  out_coeff);

}

