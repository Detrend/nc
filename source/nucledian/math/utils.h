// Project Nucledian Source File
#pragma once

#include <types.h>
#include <numbers>

namespace nc
{

/**
 * Euclidian remained of `value` divided by `range`. Results is always non-negative and lies in the interval
 * [0, range).
 * 
 * It is equivalent to the following mathematical operation: value - range * floor(value / range).
 * 
 * Example usage:
 * f32 result  = rem_euclid( 7.5f, 3.0f); // result  = 1.5f
 * f32 result2 = rem_euclid(-2.0f, 3.0f); // result2 = 1.0f
 */
f32 rem_euclid(f32 value, f32 range);

// Returns 1.0f if the value is larger than zero.
// Returns -1.0f if the value is smaller than zero.
// Returns 0.0f if the value is equal to zero.
f32 sgn(f32 value);

// Returns true if abs(num) < tolerance
bool is_zero(f32 num, f32 tolerance);

// Converts degrees to radians
f32 deg2rad(f32 degrees);

// Converts radians to degrees
f32 rad2deg(f32 radians);

// Moves the value towards the target by the amount. If the difference is
// smaller than amount then sets the value to target.
void lerp_towards(f32& value, f32 target, f32 amount);

constexpr f32 PI         = std::numbers::pi_v<f32>;
constexpr f32 HALF_PI    = PI * 0.5f;
constexpr f32 QUARTER_PI = PI * 0.25f;
constexpr f32 PI2        = PI * 2.0f;


// Combine hashes of two elements into a single hash
size_t hash_combine(size_t accumulator, size_t next_hash);

}

