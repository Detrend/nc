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
