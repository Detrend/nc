// Project Nucledian Source File
#include <vec.h>
#include <mat.h>
#include <common.h>

#include <immintrin.h>
#include <xmmintrin.h>
#include <pmmintrin.h>

#include <type_traits>

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
// <hack
template<typename T>
struct m128_friendly
{
  static constexpr bool v = false;
};

template<>
struct m128_friendly<vec4>
{
  static constexpr bool v = true;
};

template<>
struct m128_friendly<vec3>
{
  static constexpr bool v = true;
};

template<typename T>
constexpr bool m128_friendly_v = m128_friendly<T>::v;
// /hack>
//==============================================================================

//==============================================================================
template<typename T>
__m128 vec_2_m128(const T& v);

//==============================================================================
template<>
__m128 vec_2_m128(const vec4& v)
{
  return _mm_set_ps(v.x, v.y, v.z, v.w);
}

//==============================================================================
template<>
__m128 vec_2_m128(const vec3& v)
{
  return _mm_set_ps(v.x, v.y, v.z, 0.0f);
}

//==============================================================================
template<typename T>
T m128_2_vec(const __m128& reg);

//==============================================================================
template<>
vec4 m128_2_vec(const __m128& reg)
{
  vec4 v;
  _mm_store_ps(&v.x, reg);
  return v;
}

//==============================================================================
template<>
vec3 m128_2_vec(const __m128& reg)
{
  vec4 v;
  _mm_store_ps(&v.x, reg);
  return vec3{v.x, v.y, v.z};
}

//==============================================================================
[[maybe_unused]] NC_FORCE_INLINE static f32 horizontal_sum(const reg128 & mul_res)
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
  if constexpr (m128_friendly_v<vec<T, SIZE>>)
  {
    return _mm_movemask_ps(_mm_cmpeq_ps(vec_2_m128(b), vec_2_m128(a))) == 0b1111;
  }
  else
  {
    return non_sse::eq(a, b);
  }
}
  
//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> min(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  if constexpr (m128_friendly_v<vec<T, SIZE>>)
  {
    return m128_2_vec<vec4>(_mm_min_ps(vec_2_m128(a), vec_2_m128(b)));
  }
  else
  {
    return non_sse::min(a, b);
  }
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> max(const vec<T, SIZE>& v1, const vec<T, SIZE>& v2)
{
  if constexpr (m128_friendly_v<vec<T, SIZE>>)
  {
    return m128_2_vec<vec4>(_mm_max_ps(vec_2_m128(v1), vec_2_m128(v2)));
  }
  else
  {
    return non_sse::max(v1, v2);
  }
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> add(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  if constexpr (m128_friendly_v<vec<T, SIZE>>)
  { 
    return m128_2_vec<vec4>(_mm_add_ps(vec_2_m128(a), vec_2_m128(b)));
  }
  else
  {
    return non_sse::add(a, b);
  }
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> sub(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  if constexpr (m128_friendly_v<vec<T, SIZE>>)
  {
    return m128_2_vec<vec<f32, SIZE>>(_mm_sub_ps(vec_2_m128(a), vec_2_m128(b)));
  }
  else
  {
    return non_sse::sub(a, b);
  }
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> mul(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  if constexpr (m128_friendly_v<vec<T, SIZE>>)
  {
    return m128_2_vec<vec<f32, SIZE>>(_mm_mul_ps(vec_2_m128(a), vec_2_m128(b)));
  }
  else
  {
    return non_sse::mul(a, b);
  }
}

//==============================================================================
template<typename T, u64 SIZE>
T dot(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  if constexpr (m128_friendly_v<vec<T, SIZE>>)
  {
    __m128 mul_res = _mm_mul_ps(vec_2_m128(a), vec_2_m128(b));
    return horizontal_sum(mul_res);
  }
  else
  {
    return non_sse::dot(a, b);
  }
}

//==============================================================================
vec4 cross(const vec4& v1, const vec4& v2)
{
  return non_sse::cross(v1, v2);
}

//==============================================================================
f32 cross(const vec2& a, const vec2& b)
{
  return non_sse::cross(a, b);
}

//==============================================================================
vec3 cross(const vec3& a, const vec3& b)
{
  return non_sse::cross(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> div(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  if constexpr (m128_friendly_v<vec<T, SIZE>>)
  {
    return m128_2_vec<vec<f32, SIZE>>(_mm_div_ps(vec_2_m128(a), vec_2_m128(b)));
  }
  else
  {
    return non_sse::div(a, b);
  }
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> normalize(const vec<T, SIZE>& a)
{
  if constexpr (m128_friendly_v<vec<T, SIZE>>)
  {
    f32 len = sse::length(a);
    NC_ASSERT(len > 0.0f);
    return sse::div(a, vec<f32, SIZE>{len});
  }
  else
  {
    return non_sse::normalize(a);
  }
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
