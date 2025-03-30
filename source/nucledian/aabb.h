// Project Nucledian Source File
#pragma once

#include <nc_math.h>

#include <initializer_list>

namespace nc
{

template<typename FT, u64 S>
struct aabb
{
  using vec_type = vec<FT, S>;

  vec_type min;
  vec_type max;

  aabb();
  aabb(std::initializer_list<vec_type> list);

  bool is_valid() const;

  void insert_point(const vec_type& p);
};

using aabb2 = aabb<f32, 2>;
using aabb3 = aabb<f32, 3>;
using aabb4 = aabb<f32, 4>;

}

