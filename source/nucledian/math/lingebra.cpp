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
  return normalize_or(vec, glm::zero<T>());
}

//==============================================================================
template<typename T>
T clamp_length(const T& vec, f32 mn, f32 mx)
{
  return normalize_or_zero(vec) * clamp(length(vec), mn, mx);
}

//==============================================================================
template<typename T>
T normalize_or(const T& vec, const T& other)
{
  return (vec != glm::zero<T>()) ? normalize(vec) : other;
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
mat4 translation(vec3 how_much)
{
  return mat4
  {
    VEC4_X,
    VEC4_Y,
    VEC4_Z,
    vec4{how_much, 1.0f},
  };
}

//==============================================================================
mat4 scaling(vec3 how_much)
{
  return mat4
  {
    VEC4_X * how_much.x,
    VEC4_Y * how_much.y,
    VEC4_Z * how_much.z,
    VEC4_W,
  };
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

template vec2 normalize_or(const vec2&, const vec2&);
template vec3 normalize_or(const vec3&, const vec3&);
template vec4 normalize_or(const vec4&, const vec4&);

template vec2 clamp_length(const vec2&, f32, f32);
template vec3 clamp_length(const vec3&, f32, f32);
template vec4 clamp_length(const vec4&, f32, f32);

}

