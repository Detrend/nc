// Project Nuclidean Source File
#pragma once
#ifdef __SLANG__
#include "types.slangh"
using namespace nc::render;
static const f32 PI = 3.14159265f;
#else
#include <types.h>
#include <math/utils.h>
#endif

namespace nc::render
{

// Marks a missing id, like a wall without a portal or a hidden surface.
static const u32 INVALID = ~0u;
// Vertical field of view, in radians.
static const f32 FOV = 70.0f / 180.0f * PI;

}
