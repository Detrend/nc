// Project Nucledian Source File
#include <common.h>

#include <math/frustum3.h>
#include <math/lingebra.h>
#include <math/utils.h>     // is_zero

#include <intersect.h>

namespace nc
{

//==============================================================================
void Frustum3::get_frustum_edges(vec2& left, vec2& right) const
{
  const auto inv = inverse(this->matrix);
  const auto ll = inv * vec4{-1.0f, 0.0f, 1.0f, 0.0f};
  const auto rr = inv * vec4{ 1.0f, 0.0f, 1.0f, 0.0f};

  left  = normalize(ll.xz());
  right = normalize(rr.xz());
}

//==============================================================================
vec3 Frustum3::get_center() const
{
  
}

//==============================================================================
bool Frustum3::overlaps_with(const Frustum3& other) const
{
  NC_ASSERT(other.matrix == this->matrix);
  NC_ASSERT(other.bbox.is_valid() && this->bbox.is_valid());
  return intersect::aabb_aabb_2d(this->bbox, other.bbox);
}

//==============================================================================
bool Frustum3::is_valid() const
{
  return this->matrix != mat4{0};
}

//==============================================================================
static void intersect_segment_front_plane(vec3 pt, vec3 dir, f32& out_t)
{
  // solve equation
  //  a.z + d.z * t = -1.0f
  //  d.z * t = -1.0f - a.z
  //  t = -(1.0f + a.z) / d.z
  if (dir.z == 0.0f)
  {
    NC_ERROR();
    return;
  }

  out_t = -(1.0f + pt.z) / dir.z;
}

//==============================================================================
// Cuts the projected triangle by a front plane in unit distance and outputs
// either 3-4 new points or 0 points
u64 cut_by_a_front_plane(vec3 a, vec3 b, vec3 c, vec3* out)
{
  const vec3 points[] = {a, b, c};

  u64 in_front_cnt = 0;
  u64 in_front_idx = 69;
  u64 in_back_idx  = 420;

  for (const auto& pt_idx : {0, 1, 2})
  {
    const bool in_front = points[pt_idx].z <= -1.0f;
    in_front_cnt += in_front;
    in_front_idx  = in_front ? pt_idx : in_front_idx;
    in_back_idx   = in_front ? in_back_idx : pt_idx;
  }

  switch(in_front_cnt)
  {
    // All are behind us
    case 0:
    {
      return 0;
    }

    // One is in front, 2 are behind
    case 1:
    {
      NC_ASSERT(in_front_idx <= 2);

      const u64 idx1 = (in_front_idx + 1) % 3;
      const u64 idx2 = (in_front_idx + 2) % 3;

      const auto me = points[in_front_idx];
      const auto to_p1 = points[idx1]-me;
      const auto to_p2 = points[idx2]-me;

      f32 t1, t2;
      intersect_segment_front_plane(me, to_p1, t1);
      intersect_segment_front_plane(me, to_p2, t2);

      const vec3 out1 = me + to_p1 * t1;
      const vec3 out2 = me + to_p2 * t2;

      out[0] = me;
      out[1] = out1;
      out[2] = out2;

      return 3;
    }

    // Two are in front, one is behind
    case 2:
    {
      NC_ASSERT(in_back_idx <= 2);

      const u64 idx1 = (in_back_idx + 1) % 3;
      const u64 idx2 = (in_back_idx + 2) % 3;

      const auto me = points[in_back_idx];
      const auto to_p1 = points[idx1]-me;
      const auto to_p2 = points[idx2]-me;

      f32 t1, t2;
      intersect_segment_front_plane(me, to_p1, t1);
      intersect_segment_front_plane(me, to_p2, t2);

      const vec3 out1 = me + to_p1 * t1;
      const vec3 out2 = me + to_p2 * t2;

      out[0] = out1;
      out[1] = out2;
      out[2] = points[idx1];
      out[3] = points[idx2];

      return 4;
    }

    // All are in front
    case 3:
    {
      out[0] = a;
      out[1] = b;
      out[2] = c;
      return 3;
    }

    // Will never happen
    default:
    {
      NC_ERROR();
      return 0;
    }
  }
}

//==============================================================================
bool Frustum3::try_intersect_with_portal(
  vec3 p1,
  vec3 p2,
  vec3 p3,
  vec3 p4)
{
  const vec3 pt1 = matrix * vec4{p1, 1.0f};
  const vec3 pt2 = matrix * vec4{p2, 1.0f};
  const vec3 pt3 = matrix * vec4{p3, 1.0f};
  const vec3 pt4 = matrix * vec4{p3, 1.0f};

  vec3 points[8]{};
  u64  count = 0;
  count += cut_by_a_front_plane(pt1, pt2, pt3, &points[count]);
  count += cut_by_a_front_plane(pt1, pt3, pt4, &points[count]);

  aabb2 bbox2d;
  for (u64 i = 0; i < count; ++i)
  {
    const auto& point = points[i];
    NC_ASSERT(point.z <= -1.0f);
    const auto projected_point = clamp(point.xy() / -point.z, vec2{-1}, vec2{1});
    bbox2d.insert_point(projected_point);
  }

  if (!bbox2d.is_valid() || !intersect::aabb_aabb_2d(bbox2d, this->bbox))
  {
    return false;
  }

  this->bbox.insert_point(bbox2d.min);
  this->bbox.insert_point(bbox2d.max);
  return true;
}

//==============================================================================
void Frustum3::merge_with(const Frustum3& other)
{
  NC_ASSERT(other.is_valid());
  NC_ASSERT(other.bbox.is_valid());
  NC_ASSERT(!this->is_valid() || other.matrix == this->matrix);

  if (!this->is_valid())
  {
    this->matrix = other.matrix;
  }

  this->bbox.insert_point(other.bbox.min);
  this->bbox.insert_point(other.bbox.max);
}

//==============================================================================
bool Frustum3::is_empty() const
{
  return !bbox.is_valid();
}

//==============================================================================
Frustum3 Frustum3::from_look_at_and_perspective(
  const mat4& look_at,
  const mat4& persp)
{
  return Frustum3
  {
    .matrix = persp * look_at,
    .bbox   = aabb2{vec2{-1}, vec2{1}},
  };
}

//==============================================================================
Frustum3 Frustum3::from_position_direction_and_fov(
  vec3 pos,
  vec3 dir,
  vec3 up,
  f32  fov_hor,
  f32  fov_ver)
{
  const auto look = lookAt(pos, pos+dir, up);
  const auto proj = frustum(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);

  return Frustum3
  {
    .matrix = proj * look,
    .bbox   = aabb2{vec2{-1}, vec2{1}}
  };
}

//==============================================================================
void FrustumBuffer3::insert_frustum(const Frustum3& new_frustum)
{
  for (auto& slot : this->slots)
  {
    if (!slot.is_valid() || slot.overlaps_with(new_frustum))
    {
      slot.merge_with(new_frustum);
      return;
    }
  }

  this->slots[0].merge_with(new_frustum);
}

}