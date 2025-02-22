// Project Nucledian Source File
#include <vector_maths.h>
#include <cmath>                  // std::abs

namespace nc::sse
{
#include <vector_maths_declarations.h>
}

namespace nc::non_sse
{
#include <vector_maths_declarations.h>
}

namespace nc
{

//==============================================================================
template<typename T, u64 SIZE>
bool eq(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::eq(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> add(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::add(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> sub(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::sub(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> min(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::min(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> max(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::max(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> mul(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::mul(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> div(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::div(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
T dot(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::dot(a, b);
}

//==============================================================================
vec3 cross(const vec3& a, const vec3& b)
{
  return sse::cross(a, b);
}

//==============================================================================
vec4 cross(const vec4& a, const vec4& b)
{
  return sse::cross(a, b);
}

//==============================================================================
f32 cross(const vec2& a, const vec2& b)
{
  return sse::cross(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> normalize(const vec<T, SIZE>& a)
{
  return sse::normalize(a);
}

//==============================================================================
template<typename T, u64 SIZE>
T length(const vec<T, SIZE>& a)
{
  return sse::length(a);
}

}

namespace nc
{
  
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

}

//==============================================================================
//                          COMMON INSTANTIATIONS                             //
//==============================================================================
namespace nc
{
#include <vector_maths_instantiations.h>
}

namespace nc
{
template bool is_normal<f32, 4>(const vec4&, f32);
template bool is_normal<f32, 3>(const vec3&, f32);
template bool is_normal<f32, 2>(const vec2&, f32);

template bool is_zero<f32, 4>(const vec4&, f32);
template bool is_zero<f32, 3>(const vec3&, f32);
template bool is_zero<f32, 2>(const vec2&, f32);
}

