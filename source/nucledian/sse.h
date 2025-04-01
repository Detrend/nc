// Project Nucledian Source File
#pragma once

#include <types.h>
#include <common.h>

#include <immintrin.h>
#include <xmmintrin.h>

namespace nc::sse
{

#ifdef NC_MSVC
#pragma warning(push)
#pragma warning(disable: 4201)
#endif

// ================================= //
//               REG128              //
// ================================= //
// 128 bit registry helper for use of SSE instructions
union reg128
{
  using reg_type = __m128;

  // original intrinsics type
  reg_type imm;

  constexpr reg128(reg_type i) : imm(i) {};

  constexpr operator reg_type& ()
  {
    return imm;
  }

  constexpr operator reg_type() const
  {
    return imm;
  }

  // 32bit floating point
  f32 f32arr[4];
  struct
  {
    f32 f32x;
    f32 f32y;
    f32 f32z;
    f32 f32w;
  };
  constexpr reg128(f32 x, f32 y, f32 z, f32 w) : f32x(x), f32y(y), f32z(z), f32w(w) {};

  // 64 bit floating point
  f64 f64arr[2];
  struct
  {
    f64 f64x;
    f64 f64y;
  };
  constexpr reg128(f64 x, f64 y) : f64x(x), f64y(y) {};

  // unsigned ints
  u8  u8arr[16];
  u16 u16arr[8];
  u32 u32arr[4];
  u64 u64arr[2];

  // signed ints
  s8  s8arr[16];
  s16 s16arr[8];
  s32 s32arr[4];
  s64 s64arr[2];
};
static_assert(sizeof(reg128) == sizeof(__m128));


// ================================= //
//               REG64               //
// ================================= //
// 64 bit registry helper for use of SSE instructions
union reg64
{
  using reg_type = __m64;

  reg_type imm;

  operator reg_type& ()
  {
    return imm;
  }

  operator reg_type() const
  {
    return imm;
  }

  constexpr reg64(reg_type i) : imm(i) {};

  f32 f32arr[2];
  struct
  {
    f32 f32x;
    f32 f32y;
  };

  constexpr reg64(f32 x, f32 y) : f32x(x), f32y(y) {};
};
static_assert(sizeof(reg64) == sizeof(__m64));

#ifdef NC_MSVC
#pragma warning(pop)
#endif
}
