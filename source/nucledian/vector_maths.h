// Project Nucledian Source File
#pragma once

#include <vec.h>

// These are implemented both in SSE and NON-SSE versions so we can pick better
// version for each implementation.
namespace nc
{

#include <vector_maths_declarations.h>

}

// Helper functions that do not need separate sse/non-sse versions should be here.
namespace nc
{

// This one is extra because it does not have different SSE/NON-SSE implementation.
// Checks if the vector has length of 1
template<typename T, u64 SIZE>
bool is_normal(const vec<T, SIZE>& v, T threshold = static_cast<T>(0.01));

template<typename T, u64 SIZE>
bool is_zero(const vec<T, SIZE>& v, T threshold = static_cast<T>(0.01));

// Returns a vector rotated 90 degrees to the left
// (flips the components and negates x of the new vector)
vec2 flipped(const vec2& v);

}

