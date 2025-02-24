#include "temp_math.h"
#pragma once

namespace nc
{

//==============================================================================
constexpr vec3 cross(const vec3& vec1, const vec3& vec2)
{
	return std::bit_cast<vec3>(glm::cross(std::bit_cast<glm::vec3>(vec1), std::bit_cast<glm::vec3>(vec2)));
}

//==============================================================================
constexpr f32 clamp(f32 x, f32 min, f32 max)
{
	return glm::clamp(x, min, max);
}

//==============================================================================
constexpr f32 radians(f32 degrees)
{
    return glm::radians(degrees);
}

//==============================================================================
constexpr f32 rem_euclid(f32 value, f32 range)
{
    while (value >= range)
    {
        value -= range;
    }
    while (value < 0.0f)
    {
        value += range;
    }

    return value;
}

//==============================================================================
constexpr vec3 operator*(f32 x, const vec3& vec)
{
    return std::bit_cast<vec3>(x * std::bit_cast<glm::vec3>(vec));
}

//==============================================================================
constexpr vec3 operator*(const vec3& vec, f32 x)
{
    return std::bit_cast<vec3>(std::bit_cast<glm::vec3>(vec) * x);
}

//==============================================================================
constexpr vec3& operator+=(vec3& vec, const vec3& other)
{
    vec = std::bit_cast<vec3>(std::bit_cast<glm::vec3>(vec) + std::bit_cast<glm::vec3>(other));
    return vec;
}

}
