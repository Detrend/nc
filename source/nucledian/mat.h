// Project Nucledian Source File
#pragma once

#include <types.h>
#include <sse.h>

namespace nc
{

template<typename FT, u64 H, u64 W>
struct mat;

// 2x2 matrix
template<typename FT>
struct alignas(sizeof(FT) * 4) mat<FT, 2, 2>
{
  
};

// 3x3 matrix
template<typename FT>
struct mat<FT, 3, 3>
{
  
};

// 4x4 matrix
template<typename FT>
struct alignas(sizeof(FT) * 4) mat<FT, 4, 4>
{
  
};

using mat2 = mat<f32, 2, 2>;
using mat3 = mat<f32, 3, 3>;
using mat4 = mat<f32, 4, 4>;

}

