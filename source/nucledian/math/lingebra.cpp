// Project Nucledian Source File
#include <math/lingebra.h>

#include <math/utils.h>
#include <cmath>

namespace nc
{
  
//==============================================================================
template<typename T>
T normalize_or_zero(const T& vec)
{
  constexpr auto ZER0 = glm::zero<T>();
  return (vec != ZER0) ? normalize(vec) : ZER0;
}

//==============================================================================
template<typename T>
bool is_normal(const T& v, typename T::value_type threshold)
{
  constexpr auto ONE = static_cast<typename T::value_type>(1);
  return std::abs(length(v) - ONE) < threshold;
}

//==============================================================================
template<typename T>
bool is_zero(const T& v, typename T::value_type threshold)
{
  return std::abs(length(v)) < threshold;
}

//==============================================================================
vec2 flipped(const vec2& v)
{
  return vec2{-v.y, v.x};
}

//==============================================================================
f32 cross(const vec2& a, const vec2& b)
{
  return a.x * b.y - a.y * b.x;
}

//==============================================================================
template bool is_normal(const vec4&, f32);
template bool is_normal(const vec3&, f32);
template bool is_normal(const vec2&, f32);

template bool is_zero(const vec4&, f32);
template bool is_zero(const vec3&, f32);
template bool is_zero(const vec2&, f32);

template vec2 normalize_or_zero(const vec2&);
template vec3 normalize_or_zero(const vec3&);
template vec4 normalize_or_zero(const vec4&);

}

