// Project Nucledian Source File
#include <math/vector.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <glm/glm.hpp>

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
template<typename T, u64 SIZE>
bool is_normal(const vec<T, SIZE>& v, T threshold /*= static_cast<T>(0.01)*/)
{
  return std::abs(length(v) - static_cast<T>(1)) < threshold;
}

//==============================================================================
template<typename T, u64 SIZE>
bool is_zero(const vec<T, SIZE>& v, T threshold /*= static_cast<T>(0.01)*/)
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

