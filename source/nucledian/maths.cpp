// Project Nucledian Source File
#include <maths.h>
#include <algorithm>

#include <numbers>    // std::pi_v<f32>

namespace nc
{

//==============================================================================
f32 sgn(f32 value)
{
  return value == 0.0f ? 0.0f : (value > 0.0f ? 1.0f : -1.0f);
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
  return degrees * half_circle_inv * std::numbers::pi_v<f32>;
}

//==============================================================================
f32 rad2deg(f32 radians)
{
  constexpr f32 pi_inv = 1.0f / std::numbers::pi_v<f32>;
  return radians * pi_inv * 180.0f;
}

}

