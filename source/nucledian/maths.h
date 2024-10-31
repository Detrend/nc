// Project Nucledian Source File
#pragma once

#include <types.h>

namespace nc
{

// Returns 1.0f if the value is larger than zero.
// Returns -1.0f if the value is smaller than zero.
// Returns 0.0f if the value is equal to zero.
f32 sgn(f32 value);

// Returns true if abs(num) < tolerance
bool is_zero(f32 num, f32 tolerance);

}

