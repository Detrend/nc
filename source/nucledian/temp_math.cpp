// Project Nucledian Source File
#define GLM_ENABLE_EXPERIMENTAL

#include "temp_math.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <bit>

namespace nc
{

//==============================================================================
vec3 normalize(const vec3& vec)
{
  return std::bit_cast<vec3>(glm::normalize(std::bit_cast<glm::vec3>(vec)));
}

//==============================================================================
vec3 normalize_or_zero(const vec3& vec)
{
  // TODO: check for infinite vector
  // TODO: should probsbly return zero also for vectors very close to zero
  return (vec != vec3::ZERO) ? normalize(vec) : vec3::ZERO;
}

//==============================================================================
mat4 look_at(const vec3& pos, const vec3& target, const vec3& up) {
  return std::bit_cast<mat4>(glm::lookAt
  (
    std::bit_cast<glm::vec3>(pos),
    std::bit_cast<glm::vec3>(target),
    std::bit_cast<glm::vec3>(up)
  ));
}

//==============================================================================
mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far)
{
  return std::bit_cast<mat4>(glm::perspective(fov, aspect, near, far));
}

//==============================================================================
quat angleAxis(f32 angle, const vec3& vec)
{
  return std::bit_cast<quat>(glm::angleAxis(angle, std::bit_cast<glm::vec3>(vec)));
}

//==============================================================================
f32 length(const vec2& vec)
{
  return glm::length(std::bit_cast<glm::vec2>(vec));
}

//==============================================================================
f32 length2(const vec2& vec)
{
  return glm::length2(std::bit_cast<glm::vec2>(vec));
}

//==============================================================================
mat4 translate(const mat4& matrix, const vec3& vec)
{
  return std::bit_cast<mat4>(glm::translate(std::bit_cast<glm::mat4>(matrix), std::bit_cast<glm::vec3>(vec)));
}

//==============================================================================
mat4 rotate(const mat4& matrix, f32 angle, const vec3& axis)
{
  return std::bit_cast<mat4>(glm::rotate(std::bit_cast<glm::mat4>(matrix), angle, std::bit_cast<glm::vec3>(axis)));
}

//==============================================================================
mat4 scale(const mat4& matrix, const vec3& vec)
{
  return std::bit_cast<mat4>(glm::scale(std::bit_cast<glm::mat4>(matrix), std::bit_cast<glm::vec3>(vec)));
}

//==============================================================================
f32 sin(f32 x)
{
  return glm::sin(x);
}

//==============================================================================
f32 cos(f32 x)
{
  return glm::cos(x);
}

//==============================================================================
f32 rem_euclid(f32 value, f32 range)
{
  return value - range * glm::floor(value / range);
}

//==============================================================================
vec3 operator *(const quat& q, const vec3& vec)
{
  return std::bit_cast<vec3>(q * std::bit_cast<glm::vec3>(vec));
}

//==============================================================================
vec3 operator *(const vec3& vec, const quat& q)
{
  return std::bit_cast<vec3>(std::bit_cast<glm::vec3>(vec) * q);
}

//==============================================================================
const f32* value_ptr(const mat4& m)
{
  return glm::value_ptr(m);
}

}
