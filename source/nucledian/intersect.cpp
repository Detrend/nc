// Project Nucledian Source File
#include <intersect.h>
#include <maths.h>
#include <vector_maths.h>
#include <aabb.h>

#include <limits>     // FLT_MAX
#include <algorithm>  // std::min, std::max, std::abs
#include <array>
#include <cmath>

namespace nc::intersect
{

//==============================================================================
bool segment_segment(
  vec2 start_a,
  vec2 end_a,
  vec2 start_b,
  vec2 end_b)
{
  constexpr f32 TOLERANCE = 0.0001f;   // 0.1mm

  const vec2 dir_a  = end_a - start_a;
  const vec2 dir_b  = end_b - start_b;
  const f32  top    = cross(dir_b, start_b - start_a);
  const f32  bottom = cross(dir_b, dir_a);

  NC_ASSERT(length(dir_a) > TOLERANCE);
  NC_ASSERT(length(dir_b) > TOLERANCE);

  const bool tp_zero = is_zero(top,    TOLERANCE);
  const bool bt_zero = is_zero(bottom, TOLERANCE);

  if (tp_zero && bt_zero)
  {
    // parallel and lines are intersecting, check for segment
    // intersection by projecting onto an interval
    // TODO: can be probably done faster
    // TODO: maybe no need to normalize?
    const vec2 projection_plane = /*normalize(*/dir_a/*)*/;

    const f32 a1 = dot(start_a, projection_plane);
    const f32 a2 = dot(end_a,   projection_plane);
    const f32 b1 = dot(start_b, projection_plane);
    const f32 b2 = dot(end_b,   projection_plane);

    const f32 imin = std::max(std::min(a1, a2), std::min(b1, b2));
    const f32 imax = std::min(std::max(a1, a2), std::max(b1, b2));

    return imax >= imin;
  }
  else if (bt_zero)
  {
    // parallel and non-intersecting
    return false;
  }
  else
  {
    const f32 t = top / bottom;

    if (t >= 0 && t <= 1)
    {
      // might be intersecting, check for u as well
      const f32 utop    = cross(dir_a, start_a - start_b);
      const f32 ubottom = -bottom;

      // This should not happen AFAIK
      NC_ASSERT(!is_zero(ubottom, TOLERANCE));

      const f32 u = utop / ubottom;
      return u >= 0 && u <= 1;
    }

    // lines intersect, but segments do not
    return false;
  }
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
bool point_triangle(vec2 p, vec2 a, vec2 b, vec2 c)
{
  const auto a_to_b = b-a;
  const auto b_to_c = c-b;
  const auto c_to_a = a-c;

  const auto a_to_p = p-a;
  const auto b_to_p = p-b;
  const auto c_to_p = p-c;

  const f32 gl_sign = sgn(cross(a_to_b, b_to_c));

  const f32 a_sign = sgn(cross(a_to_b, a_to_p));
  const f32 b_sign = sgn(cross(b_to_c, b_to_p));
  const f32 c_sign = sgn(cross(c_to_a, c_to_p));

  auto eq_or_zero = [](f32 f1, f32 f2)
  {
    return f1 == f2 || f2 == 0.0f;
  };

  return eq_or_zero(gl_sign, a_sign)
      && eq_or_zero(gl_sign, b_sign)
      && eq_or_zero(gl_sign, c_sign);
}

//==============================================================================
bool triangle_triangle(vec2 a1, vec2 b1, vec2 c1, vec2 a2, vec2 b2, vec2 c2)
{
  auto arr1 = std::array{a1, b1, c1};
  auto arr2 = std::array{a2, b2, c2};
  return convex_convex(
    std::span{arr1.data(), arr1.size()},
    std::span{arr2.data(), arr2.size()});
}

//==============================================================================
bool convex_convex(std::span<vec2> a, std::span<vec2> b, f32 threshold)
{
  // iterate edges of both shapes and search for a line that separates
  // both of them
  const std::span<vec2> points[2] = {a, b};

  // shapes must have at least 3 points
  if (points[0].size() < 3 || points[1].size() < 3)
  {
    return false;
  }

  for (u8 shape_idx = 0; shape_idx < 2; ++shape_idx)
  {
    for (u8 side_idx = 0; side_idx < points[shape_idx].size(); ++side_idx)
    {
      const u8 next_side_idx = (side_idx + 1) % points[shape_idx].size();

      const auto& p1 = points[shape_idx][side_idx];
      const auto& p2 = points[shape_idx][next_side_idx];
      const auto  line   = normalize(p1-p2);
      const auto  normal = vec2{line.y, -line.x};

      struct LocalInterval
      {
        f32 min =  FLT_MAX;
        f32 max = -FLT_MAX;   // FLT_MIN is a trap..
      };

      LocalInterval intervals[2]{};

      // now project all points onto the normal and create
      // an interval for each of the two triangles - if the
      // intervals overlap then the triangles MIGHT overlap,
      // but we must check all other hyperplanes as well
      for (u8 pset_idx = 0; pset_idx < 2; ++pset_idx)
      {
        for (u8 point_idx = 0; point_idx < points[pset_idx].size(); ++point_idx)
        {
          const f32 projected = dot(points[pset_idx][point_idx], normal);

          // update min/max of the interval
          auto& interval = intervals[pset_idx];
          interval.min = std::min(interval.min, projected);
          interval.max = std::max(interval.max, projected);
        }
      }

      const f32 interval_min = std::max(intervals[0].min, intervals[1].min);
      const f32 interval_max = std::min(intervals[0].max, intervals[1].max);

      // if the shapes have an intersection smaller than 1cm
      // then we pretend they do not intersect
      if (interval_max - interval_min < threshold)
      {
        // if we got here than it means that we found a hyperplane that separates
        // both triangles from each other and therefore they do not intersect
        return false;
      }

      // plane not found, go on
      // TODO: finish this!!!
    }
  }

  // there is no hyperplane that separates the two shapes, therefore
  // they must intersect
  return true;
}

}

namespace nc
{

//==============================================================================
bool Frustum2::contains_point(vec2 p) const
{
  NC_ASSERT(is_normal(this->direction));

  if (is_full()) [[unlikely]]
  {
    return true;
  }

  const auto to_point = p-center;

  if (length(to_point) == 0.0f)
  {
    return true;
  }

  const auto projected = dot(normalize(to_point), direction);

  // Use > instead od >= for cases when the frustum is empty
  return projected > this->angle;
}

//==============================================================================
bool Frustum2::intersects_segment(vec2 p1, vec2 p2) const
{
  NC_ASSERT(is_normal(this->direction));

  if (this->is_empty()) [[unlikely]]
  {
    return false;
  }

  if (this->is_full())  [[unlikely]]
  {
    return true;
  }

  if (this->contains_point(p1) || this->contains_point(p2))
  {
    return true;
  }

  const auto to_p1 = normalize(p1-this->center);
  const auto to_p2 = normalize(p2-this->center);
  const auto sgn1  = sgn(cross(to_p1, this->direction));
  const auto sgn2  = sgn(cross(to_p2, this->direction));

  if (sgn1 == sgn2)
  {
    // both points of the wall are on the same side,
    // therefore the wall does not intersect with
    // the frustum
    return false;
  }

  // intersect the direction and wall
  const auto p1_to_p2     = p2-p1;
  const auto center_to_p1 = p1-this->center;
  const auto top = cross(p1_to_p2, center_to_p1);
  const auto bot = cross(p1_to_p2, this->direction);

  // should never happen as both points are on different sides
  NC_ASSERT(bot != 0.0f);

  // if t is 0 or more then the wall intersects with the direction
  // line and therefore is contained in the frustum
  const auto t = top / bot;
  return t >= 0.0f;
}

//==============================================================================
bool Frustum2::is_full() const
{
  return this->angle <= FULL_ANGLE;
}

//==============================================================================
bool Frustum2::is_empty() const
{
  return this->angle >= EMPTY_ANGLE;
}

//==============================================================================
static auto get_edges(const Frustum2& f)
{
  struct Edges
  {
    vec2 e1;
    vec2 e2;
  };

  const auto dir = f.direction;
  const auto flp = flipped(dir);
  const auto a   = f.angle;
  const auto b   = std::sqrt(1.0f - a * a);

  const auto l_edge = dir * a + flp * b;
  const auto r_edge = dir * a - flp * b;

  return Edges{.e1 = l_edge, .e2 = r_edge};
}

//==============================================================================
Frustum2 Frustum2::modied_with_portal(vec2 p1, vec2 p2) const
{
  NC_ASSERT(is_normal(this->direction));

  // This could have been done in a faster way probably..
  if (this->intersects_segment(p1, p2))
  {
    const bool inside1 = this->contains_point(p1);
    const bool inside2 = this->contains_point(p2);

    if (inside1 && inside2)
    {
      // both points inside, easy case
      const auto to_p1 = normalize(p1-this->center);
      const auto to_p2 = normalize(p2-this->center);
      const auto midir = normalize(to_p1 + to_p2);

      return Frustum2
      {
        .center    = this->center,
        .direction = midir,
        .angle     = dot(to_p1, midir),
      };
    }
    else if (!inside1 && !inside2)
    {
      // Both points outside but intersecting, easy as well.
      // Just keep the old frustum
      return *this;
    }

    const auto point_outside = inside1 ? p2 : p1;
    const auto point_inside  = inside1 ? p1 : p2;
    const auto to_inside_pt  = normalize(point_inside  - this->center);
    const bool outside_right = cross(point_outside - this->center, this->direction) > 0;

    // the two frustums are overlapping, not that easy

    const auto[l_edge, r_edge] = get_edges(*this);

    // These should end up having an unit length mathematically speaking,
    // but the floating point inaccuracy might have fucked it up..
    NC_ASSERT(is_normal(l_edge));
    NC_ASSERT(is_normal(r_edge));
    // And these should hold as well
    NC_ASSERT(is_zero(dot(l_edge, this->direction) - this->angle, 0.0001f));
    NC_ASSERT(is_zero(dot(r_edge, this->direction) - this->angle, 0.0001f));

    // we keep the edge in the direction the outside point is on..
    const auto edge_we_keep = outside_right ? r_edge : l_edge;

    const auto new_frustum_dir   = normalize(edge_we_keep + to_inside_pt);
    // can be probably calculated more efficiently..
    const auto new_frustum_angle = dot(new_frustum_dir, edge_we_keep);

    // and now construct a new Frustum from the 2 edges
    return Frustum2
    {
      .center    = this->center,
      .direction = new_frustum_dir,
      .angle     = new_frustum_angle,
    };
  }
  else
  {
    // Return empty frustum because the portal has zero
    // intersection with us
    return Frustum2
    {
      .center    = this->center,
      .direction = this->direction,
      .angle     = EMPTY_ANGLE,
    };
  }
}

//==============================================================================
Frustum2 Frustum2::merged_with(const Frustum2& other) const
{
  NC_ASSERT(other.center == this->center);

  if (other.is_full() || this->is_full()) [[unlikely]]
  {
    return Frustum2
    {
      .center    = this->center,
      .direction = this->direction,
      .angle     = FULL_ANGLE,
    };
  }

  // Calculate left and right edges of the frustums and compare them
  // TODO: can be done in a faster way using SIMD by doing 2 sqrts for a price of 1
  const auto[this_l_edge,  this_r_edge]  = get_edges(*this);
  const auto[other_l_edge, other_r_edge] = get_edges(other);

  // Should be normal
  NC_ASSERT(is_normal(this_l_edge)  && is_normal(this_r_edge));
  NC_ASSERT(is_normal(other_l_edge) && is_normal(other_r_edge));

  const f32 lr = dot(this_l_edge, other_r_edge);
  const f32 rl = dot(this_r_edge, other_l_edge);

  // Now check the smaller one
  const bool lr_smaller = lr < rl;
  const auto l_edge     = lr_smaller ? this_l_edge  : other_l_edge;
  const auto r_edge     = lr_smaller ? other_r_edge : this_r_edge;

  // negative if turned backwards, will help us flip the dir
  const auto smaller_sgn = sgn(std::min(lr, rl)); 

  // Good, then calc the dir
  auto new_direction = normalize((l_edge + r_edge) * smaller_sgn);

  if (is_zero(new_direction)) [[unlikely]]
  {
    // This happens if l_edge and r_edge point in an opposite directions.
    // We flip one of them around in an appropriate direction
    const auto l_edge2 = !lr_smaller ? this_l_edge  : other_l_edge;
    const auto r_edge2 = !lr_smaller ? other_r_edge : this_r_edge;
    const auto greater_sgn    = sgn(std::max(lr, rl)); 
    const auto new_direction2 = (l_edge2 + r_edge2) * greater_sgn;

    // This could theoretically happen if both angles are 0
    NC_ASSERT(!is_zero(new_direction2));

    new_direction = flipped(r_edge);
    // The dot product will end up being negative if the flipped vector points in a wrong direction
    new_direction = new_direction * sgn(dot(new_direction, new_direction2));
  }

  const auto new_angle = dot(l_edge, new_direction);

  return Frustum2
  {
    .center    = this->center,
    .direction = new_direction,
    .angle     = new_angle,
  };
}

//==============================================================================
f32 Frustum2::angle_difference(const Frustum2& other) const
{
  NC_ASSERT(other.center == this->center);

  // First, calculate the angle difference between directions.
  // We can do this by using dot product and asin.
  // TODO: Maybe do this without the asins?
  const f32 diff_of_directions = std::asin(dot(this->direction, other.direction));
  const f32 deg1 = std::asin(this->angle);
  const f32 deg2 = std::asin(other.angle);

  return diff_of_directions - deg1 - deg2;
}

//==============================================================================
Frustum2 Frustum2::from_point_and_portal(vec2 point, vec2 a, vec2 b)
{
  auto new_frustum = Frustum2
  {
    .center    = point,
    .direction = vec2{0, 1},
    .angle     = FULL_ANGLE,
  };

  if (point == a || point == b)
  {
    return new_frustum;
  }

  const auto to_a = normalize(a-point);
  const auto to_b = normalize(b-point);

  if (dot(to_a, to_b) == -1.0f)
  {
    // stuck in the wall, return empty frustum
    new_frustum.angle = 1.0f;
    new_frustum.direction = vec2{to_a.y, to_a.x};

    NC_ASSERT(new_frustum.is_empty());

    return new_frustum;
  }

  new_frustum.direction = normalize(to_a + to_b);
  new_frustum.angle     = dot(to_a, new_frustum.direction);
  NC_ASSERT(new_frustum.angle >= 0.0f);
  return new_frustum;
}

}

