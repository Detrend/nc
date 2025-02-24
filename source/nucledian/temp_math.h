// Project Nucledian Source File
#pragma once

#include <types.h>
#include <vec.h>
#include <vector_maths.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>

namespace nc
{

using mat4 = glm::mat4;
using quat = glm::quat;

vec3 normalize(const vec3& vec);
vec3 normalize_or_zero(const vec3& vec);
mat4 look_at(const vec3& pos, const vec3& target, const vec3& up);
mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far);
constexpr vec3 cross(const vec3& vec1, const vec3& vec2);
quat angleAxis(f32 angle, const vec3& vec);

constexpr f32 clamp(f32 x, f32 min, f32 max);
f32 sin(f32 x);
f32 cos(f32 x);
constexpr f32 radians(f32 degrees);
constexpr f32 rem_euclid(f32 value, f32 range);

constexpr vec3 operator*(f32 x, const vec3& vec);
constexpr vec3 operator*(const vec3& vec, f32 x);
constexpr vec3& operator+=(vec3& vec, const vec3& other);
vec3 operator *(const quat& q, const vec3& vec);
vec3 operator *(const vec3& vec, const quat& q);

const f32* value_ptr(const mat4& m);

inline constexpr f32 pi			= glm::pi<f32>();
inline constexpr f32 half_pi	= glm::half_pi<f32>();
inline constexpr f32 quarter_pi = glm::quarter_pi<f32>();

}

#include <temp_math.inl>