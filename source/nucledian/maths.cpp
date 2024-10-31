// Project Nucledian Source File
#include <maths.h>
#include <algorithm>

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

}

