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

}

