// Project Nucledian Source File
#pragma once

#include <types.h>
#include <vector_maths.h>
#include <aabb.h>

namespace nc::intersect
{

bool segment_segment_2d(vec2 start_a, vec2 end_a, vec2 start_b, vec2 end_b, f32& t, bool& parallel);

bool aabb_aabb_2d(const aabb2& a, const aabb2& b);

}

