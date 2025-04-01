// Project Nucledian Source File
#include <math/utils.h>

#include <cmath> // std::floor

namespace nc
{
  
//==============================================================================
f32 rem_euclid(f32 value, f32 range)
{
  return value - range * std::floor(value / range);
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
  return degrees * half_circle_inv * PI;
}

//==============================================================================
f32 rad2deg(f32 radians)
{
  constexpr f32 PI_INV = 1.0f / PI;
  return radians * PI_INV * 180.0f;
}

}

