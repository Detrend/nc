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

  constexpr vec<f32, 2> with_x(f32 new_x) const;
  constexpr vec<f32, 2> with_y(f32 new_y) const;

  static const vec<f32, 2> ZERO;
  static const vec<f32, 2> ONE;
  static const vec<f32, 2> X;
  static const vec<f32, 2> Y;
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

  constexpr vec<f32, 3> with_x(f32 new_x) const;
  constexpr vec<f32, 3> with_y(f32 new_y) const;
  constexpr vec<f32, 3> with_z(f32 new_z) const;

  // Creates vec2 from self, discarding z.
  constexpr vec<f32, 2> truncate() const;

  static const vec<f32, 3> ZERO;
  static const vec<f32, 3> ONE;
  static const vec<f32, 3> X;
  static const vec<f32, 3> Y;
  static const vec<f32, 3> Z;
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

  constexpr vec<f32, 4> with_x(f32 new_x) const;
  constexpr vec<f32, 4> with_y(f32 new_y) const;
  constexpr vec<f32, 4> with_z(f32 new_z) const;
  constexpr vec<f32, 4> with_w(f32 new_w) const;

  // Creates vec3 from self, discarding w.
  constexpr vec<f32, 3> truncate() const;

  static const vec<f32, 4> ZERO;
  static const vec<f32, 4> ONE;
  static const vec<f32, 4> X;
  static const vec<f32, 4> Y;
  static const vec<f32, 4> Z;
  static const vec<f32, 4> W;
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
vec<F, S> operator*(const vec<F, S>& a, const F& b);

template<typename F, u64 S>
vec<F, S> operator/(const vec<F, S>& a, const vec<F, S>& b);

template<typename F, u64 S>
vec<F, S> operator/(const vec<F, S>& a, const F& b);

template<typename F, u64 S>
vec<F, S> operator+(const vec<F, S>& a, const vec<F, S>& b);

}

#ifdef NC_MSVC
#pragma warning(pop)
#endif

#include <vec.inl>

namespace nc
{

inline constexpr vec2 ZERO2{0};
inline constexpr vec3 ZERO3{0};
inline constexpr vec4 ZERO4{0};

}

