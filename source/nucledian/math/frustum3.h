// Project Nucledian Source File
#pragma once

#include <config.h>

#include <types.h>
#include <math/matrix.h>
#include <aabb.h>

#include <array>

namespace nc
{

struct Frustum3
{
  mat4  matrix = mat4{0};
  aabb2 bbox   = aabb2{};

  bool try_intersect_with_portal(vec3, vec3, vec3, vec3);

  void merge_with(const Frustum3& other);

  void get_frustum_edges(vec2& left, vec2& right) const;

  vec3 get_center() const;

  bool overlaps_with(const Frustum3& other) const;
  bool is_valid()                           const;
  bool is_empty()                           const;

  static Frustum3 from_look_at_and_perspective(
    const mat4& look_at, const mat4& persp);

  static Frustum3 from_position_direction_and_fov(
    vec3 pos, vec3 dir, vec3 up, f32 fov_hor, f32 fov_ver);
};

struct FrustumBuffer3
{ 
  using Array = std::array<Frustum3, 4>;
  Array slots;

  explicit FrustumBuffer3(const Frustum3& from)
  {
    slots.fill(Frustum3{});
    slots[0] = from;
  }

  FrustumBuffer3() : FrustumBuffer3(Frustum3{}) {};

  void insert_frustum(const Frustum3& new_frustum);
};
  
}

