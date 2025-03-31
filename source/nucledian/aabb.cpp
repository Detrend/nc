// Project Nucledian Source File
#include <aabb.h>
#include <math/vector.h>
#include <math/lingebra.h>

#include <initializer_list>
#include <limits>

namespace nc
{

//==============================================================================
template<typename FT, u64 S>
aabb<FT, S>::aabb()
: min(vec_type{std::numeric_limits<FT>::max()})
, max(vec_type{std::numeric_limits<FT>::lowest()})
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
  return ::glm::min(min, max) == min && ::glm::max(min, max) == max;
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

