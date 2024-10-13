// Project Nucledian Source File
#include <aabb.h>
#include <vector_maths.h>

#include <initializer_list>
#include <limits>

namespace nc
{

//==============================================================================
template<typename FT, u64 S>
aabb<FT, S>::aabb()
: min(vec_type{std::numeric_limits<FT>::max()})
, max(vec_type{std::numeric_limits<FT>::min()})
{

}

//==============================================================================
template<typename FT, u64 S>
aabb<FT, S>::aabb(std::initializer_list<vec_type> list)
: aabb()
{
  for (auto p : list)
  {
    this->insert_point(p);
  }
}

//==============================================================================
template<typename FT, u64 S>
bool aabb<FT, S>::is_valid() const
{
  return ::nc::min(min, max) == min && ::nc::max(min, max) == max;
}

//==============================================================================
template<typename FT, u64 S>
void aabb<FT, S>::insert_point(const vec_type& p)
{
  min = ::nc::min(min, p);
  max = ::nc::max(max, p);
}

template struct aabb<f32, 2>;
template struct aabb<f32, 3>;
template struct aabb<f32, 4>;

}

