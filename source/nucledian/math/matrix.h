// Project Nucledian Source File
#pragma once

#include <types.h>
#include <math/vector.h>

#include <glm/ext/matrix_float2x2_precision.hpp>
#include <glm/ext/matrix_float3x3_precision.hpp>
#include <glm/ext/matrix_float4x4_precision.hpp>

namespace nc
{

using namespace glm;

template<typename T, u64 W, u64 H>
using mat = glm::mat<W, H, T, glm::packed>;

using mat4 = glm::mat<4, 4, f32, glm::packed>;
using mat3 = glm::mat<3, 3, f32, glm::packed>;
using mat2 = glm::mat<2, 2, f32, glm::packed>;


template<typename T, u64 W, u64 H>
using mata = glm::mat<W, H, T, glm::aligned>;

using mat4a = glm::mat<4, 4, f32, glm::aligned>;
using mat3a = glm::mat<3, 3, f32, glm::aligned>;
using mat2a = glm::mat<2, 2, f32, glm::aligned>;

}

