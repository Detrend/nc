// Project Nucledian Source File

#include <nc_math.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace nc
{

//==============================================================================
vec3 normalize_or_zero(const vec3& vec)
{
  // TODO: check for infinite vector
  // TODO: should probsbly return zero also for vectors very close to zero
  return (vec != glm::zero<vec3>()) ? normalize(vec) : glm::zero<vec3>();
}

//==============================================================================
f32 rem_euclid(f32 value, f32 range)
{
  return value - range * glm::floor(value / range);
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
vec3 with_y(const vec3& v, f32 y)
{
  return vec3{v.x, y, v.z};
}

//==============================================================================
f32 sgn(f32 value)
{
  return static_cast<f32>(value > 0.0f) - static_cast<f32>(value < 0.0f);
}

//==============================================================================
bool is_zero(f32 num, f32 tolerance)
{
  return std::abs(num) < tolerance;
}

//==============================================================================
f32 deg2rad(f32 degrees)
{
  constexpr f32 half_circle_inv = 1.0f / 180.0f;
  return degrees * half_circle_inv * pi;
}

//==============================================================================
f32 rad2deg(f32 radians)
{
  constexpr f32 pi_inv = 1.0f / pi;
  return radians * pi_inv * 180.0f;
}


//==============================================================================
template bool is_normal(const vec4&, f32);
template bool is_normal(const vec3&, f32);
template bool is_normal(const vec2&, f32);

template bool is_zero(const vec4&, f32);
template bool is_zero(const vec3&, f32);
template bool is_zero(const vec2&, f32);

}

