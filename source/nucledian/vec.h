// Project Nucledian Source File
#pragma once

#include <types.h>
#include <base_vec.h>
#include <sse.h>

#ifdef NC_MSVC
#pragma warning(push)
#pragma warning(disable: 4201)
#endif

namespace nc
{

using vec2 = vec<f32, 2>;
using vec3 = vec<f32, 3>;
using vec4 = vec<f32, 4>;

// SSE optimized vec2 type
template<>
struct vec<f32, 2>
{
  union
  {
    struct
    {
      f32 x;
      f32 y;
    };
    f32 v[2];
    sse::reg64 r;
  };

  constexpr vec(f32 v);
  constexpr vec(f32 x, f32 y);
  constexpr vec(const sse::reg64& reg);
  constexpr vec();

  const f32& operator[](u64 index) const;
  f32&       operator[](u64 index);

  operator sse::reg64&();
  operator sse::reg64() const;
};

// The vec3 type.
// Use only if you want to save space
template<>
struct vec<f32, 3>
{
  union
  {
    struct
    {
      f32 x;
      f32 y;
      f32 z;
    };
    f32 v[3];
  };

  constexpr vec(f32 v);
  constexpr vec(f32 x, f32 y, f32 z);
  constexpr vec(vec2 v2, f32 z);
  constexpr vec(const vec4& v4);
  constexpr vec();

  const f32& operator[](u64 index) const;
  f32&       operator[](u64 index);
};

// SSE optimized vector4
template<>
struct alignas(16) vec<f32, 4>
{
  union
  {
    struct
    {
      f32 x;
      f32 y;
      f32 z;
      f32 w;
    };
    f32 v[4];
    sse::reg128 r;
  };

  constexpr vec(f32 v);
  constexpr vec(f32 x, f32 y, f32 z, f32 w);
  constexpr vec(const sse::reg128& sse_reg);
  constexpr vec(vec3 v3, f32 w = 0.0f);
  constexpr vec();

  const f32& operator[](u64 index) const;
  f32&       operator[](u64 index);

  operator sse::reg128&();
  operator sse::reg128() const;
};

template<typename F, u64 S>
bool operator==(const vec<F, S>& a, const vec<F, S>& b);

template<typename F, u64 S>
vec<F, S> operator-(const vec<F, S>& a);

template<typename F, u64 S>
vec<F, S> operator-(const vec<F, S>& a, const vec<F, S>& b);

template<typename F, u64 S>
vec<F, S> operator*(const vec<F, S>& a, const vec<F, S>& b);

template<typename F, u64 S>
vec<F, S> operator/(const vec<F, S>& a, const vec<F, S>& b);

template<typename F, u64 S>
vec<F, S> operator+(const vec<F, S>& a, const vec<F, S>& b);

}

#ifdef NC_MSVC
#pragma warning(pop)
#endif

#include <vec.inl>

