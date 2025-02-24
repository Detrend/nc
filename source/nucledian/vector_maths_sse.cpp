// Project Nucledian Source File
#include <vec.h>
#include <common.h>

#include <immintrin.h>
#include <xmmintrin.h>
#include <pmmintrin.h>

namespace nc::sse
{
#include <vector_maths_declarations.h>
}

namespace nc::non_sse
{
#include <vector_maths_declarations.h>
}

namespace nc::sse
{

//==============================================================================
NC_FORCE_INLINE static f32 horizontal_sum(const reg128& mul_res)
{
  __m128 shuf_res, sums_reg;

  // Calculates the sum of SSE Register - https://stackoverflow.com/a/35270026/195787
  shuf_res = _mm_movehdup_ps(mul_res);        // Broadcast elements 3,1 to 2,0
  // shufReg = [a.y*b.y, a.y*b.y, a.w*b.w, a.w*b.w]

  sums_reg = _mm_add_ps(mul_res, shuf_res);
  // sumsReg = [a.y*b.y + a.x*b.x, a.y*b.y + a.y*b.y, a.z*b.z + a.w*b.w, a.w*b.w + a.w*b.w]

  shuf_res = _mm_movehl_ps(shuf_res, sums_reg); // High Half -> Low Half
  sums_reg = _mm_add_ss(sums_reg, shuf_res);

  return _mm_cvtss_f32(sums_reg); // Result in the lower part of the SSE Register
}

//==============================================================================
template<typename T, u64 SIZE>
bool eq(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return non_sse::eq(a, b);
}

//==============================================================================
template<>
bool eq(const vec4& a, const vec4& b)
{
  return _mm_movemask_ps(_mm_cmpeq_ps(a.r, b.r)) == 0b1111;
}

//==============================================================================
template<>
bool eq(const vec3& a, const vec3& b)
{
  return eq(vec4{a}, vec4{b});
}
  
//==============================================================================
template<>
vec4 min(const vec4& a, const vec4& b)
{
  return vec4{_mm_min_ps(a.r, b.r)};
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> min(const vec<T, SIZE>& v1, const vec<T, SIZE>& v2)
{
  return non_sse::min(v1, v2);
}

//==============================================================================
template<>
vec4 max(const vec4& a, const vec4& b)
{
  return vec4{_mm_max_ps(a.r, b.r)};
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> max(const vec<T, SIZE>& v1, const vec<T, SIZE>& v2)
{
  return non_sse::max(v1, v2);
}

//==============================================================================
template<>
vec4 add(const vec4& a, const vec4& b)
{
  return vec4{_mm_add_ps(a.r, b.r)};
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> add(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return non_sse::add(a, b);
}

//==============================================================================
template<>
vec4 sub(const vec4& a, const vec4& b)
{
  return vec4{_mm_sub_ps(a.r, b.r)};
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> sub(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return non_sse::sub(a, b);
}

//==============================================================================
template<>
vec4 mul(const vec4& a, const vec4& b)
{
  return vec4{_mm_mul_ps(a.r, b.r)};
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> mul(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return non_sse::mul(a, b);
}

//==============================================================================
template<>
f32 dot(const vec4& a, const vec4& b)
{
  __m128 mul_res;
  mul_res = _mm_mul_ps(a.r, b.r);
  // mulRes = [a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w]

  return horizontal_sum(mul_res);
}

//==============================================================================
template<typename T, u64 SIZE>
T dot(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return non_sse::dot(a, b);
}

//==============================================================================
vec4 cross(const vec4& v1, const vec4& v2)
{
  // x = a.y * b.z - a.z * b.y
  // y = a.z * b.x - a.x * b.z
  // z = a.x * b.y - a.y * b.x

  //     X            Y            Z            W

  // r1:       b.z          b.x          b.y    0
  // r2: a.y          a.z          a.x          0
  // q1: a.y * b.z    a.z * b.x    a.x * b.y    0

  // r3:       b.y          b.z          b.x    0
  // r4: a.z          a.x          a.y          0
  // q2: a.z * b.y    a.x * b.z    a.y * b.x    0

  sse::reg128 a = v1;
  sse::reg128 b = v2;

  sse::reg128 r1 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(2, 0, 1, 0));
  sse::reg128 r2 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 2, 0, 0));
  sse::reg128 r3 = _mm_shuffle_ps(b, b, _MM_SHUFFLE(1, 2, 0, 0));
  sse::reg128 r4 = _mm_shuffle_ps(a, a, _MM_SHUFFLE(2, 0, 1, 0));

  sse::reg128 q1 = _mm_mul_ps(r1, r2);
  sse::reg128 q2 = _mm_mul_ps(r3, r4);

  sse::reg128 res = _mm_sub_ps(q1, q2);

  return vec4(res.f32x, res.f32y, res.f32z, 0);
}

//==============================================================================
f32 cross(const vec2& a, const vec2& b)
{
  return a.x * b.y - a.y * b.x;
}

//==============================================================================
//vec3 cross(const vec3& a, const vec3& b)
//{
//  return cross(vec4{a}, vec4{b});
//}

//==============================================================================
template<>
vec4 div(const vec4& a, const vec4& b)
{
  return vec4{_mm_div_ps(a.r, b.r)};
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> div(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return non_sse::div(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> normalize(const vec<T, SIZE>& a)
{
  return non_sse::normalize(a);
}

//==============================================================================
template<typename T, u64 SIZE>
T length(const vec<T, SIZE>& a)
{
  return non_sse::length(a);
}

}

namespace nc::sse
{
#include <vector_maths_instantiations.h>
}
