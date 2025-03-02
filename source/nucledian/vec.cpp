// Project Nucledian Source File
#include <vec.h>

#include <vector_maths.h>

namespace nc
{

//==============================================================================
template<typename F, u64 S>
bool operator==(const vec<F, S>& a, const vec<F, S>& b)
{
  return eq(a, b);
}

//==============================================================================
template<typename F, u64 S>
vec<F, S> operator-(const vec<F, S>& a)
{
  return mul(a, vec<F, S>(-1));
}

//==============================================================================
template<typename F, u64 S>
vec<F, S> operator-(const vec<F, S>& a, const vec<F, S>& b)
{
  return sub(a, b);
}

//==============================================================================
template<typename F, u64 S>
vec<F, S>& operator-=(vec<F, S>& a, const vec<F, S>& b)
{
  return a = a - b;
}

//==============================================================================
template<typename F, u64 S>
vec<F, S>& operator+=(vec<F, S>& a, const vec<F, S>& b)
{
  return a = a + b;
}

//==============================================================================
template<typename F, u64 S>
vec<F, S>& operator/=(vec<F, S>& a, const vec<F, S>& b)
{
  return a = a / b;
}

//==============================================================================
template<typename F, u64 S>
vec<F, S>& operator/=(vec<F, S>& a, const F& b)
{
  return a = a / b;
}
 
//==============================================================================
template<typename F, u64 S>
vec<F, S>& operator*=(vec<F, S>& a, const F& b)
{
  return a = a * b;
}

//==============================================================================
template<typename F, u64 S>
vec<F, S>& operator*=(vec<F, S>& a, const vec<F, S>& b)
{
 return a = a * b;
}

//==============================================================================
template<typename F, u64 S>
vec<F, S> operator*(const vec<F, S>& a, const vec<F, S>& b)
{
  return mul(a, b);
}

//==============================================================================
template<typename F, u64 S>
vec<F, S> operator*(const F& a, const vec<F, S>& b)
{
  return operator*(b, a);
}

//==============================================================================
template<typename F, u64 S>
vec<F, S> operator*(const vec<F, S>& a, const F& b)
{
  return mul(a, vec<F, S>{b});
}

//==============================================================================
template<typename F, u64 S>
vec<F, S> operator/(const vec<F, S>& a, const vec<F, S>& b)
{
  return div(a, b);
}

//==============================================================================
template<typename F, u64 S>
vec<F, S> operator/(const vec<F, S>& a, const F& b)
{
  return div(a, vec<F, S>{b});
}

//==============================================================================
template<typename F, u64 S>
vec<F, S> operator+(const vec<F, S>& a, const vec<F, S>& b)
{
  return add(a, b);
}

//==============================================================================
template bool operator==(const vec4&, const vec4&);
template bool operator==(const vec3&, const vec3&);
template bool operator==(const vec2&, const vec2&);

template vec4 operator+(const vec4&, const vec4&);
template vec3 operator+(const vec3&, const vec3&);
template vec2 operator+(const vec2&, const vec2&);

template vec4 operator-(const vec4&, const vec4&);
template vec3 operator-(const vec3&, const vec3&);
template vec2 operator-(const vec2&, const vec2&);

template vec4 operator*(const vec4&, const vec4&);
template vec3 operator*(const vec3&, const vec3&);
template vec2 operator*(const vec2&, const vec2&);

template vec4 operator*(const vec4&, const f32&);
template vec3 operator*(const vec3&, const f32&);
template vec2 operator*(const vec2&, const f32&);

template vec4 operator*(const f32&, const vec4&);
template vec3 operator*(const f32&, const vec3&);
template vec2 operator*(const f32&, const vec2&);

template vec4 operator/(const vec4&, const vec4&);
template vec3 operator/(const vec3&, const vec3&);
template vec2 operator/(const vec2&, const vec2&);

template vec4 operator/(const vec4&, const f32&);
template vec3 operator/(const vec3&, const f32&);
template vec2 operator/(const vec2&, const f32&);

template vec4 operator-(const vec4&);
template vec3 operator-(const vec3&);
template vec2 operator-(const vec2&);

template vec4& operator+=(vec4&, const vec4&);
template vec3& operator+=(vec3&, const vec3&);
template vec2& operator+=(vec2&, const vec2&);

template vec4& operator-=(vec4&, const vec4&);
template vec3& operator-=(vec3&, const vec3&);
template vec2& operator-=(vec2&, const vec2&);

template vec4& operator*=(vec4&, const vec4&);
template vec3& operator*=(vec3&, const vec3&);
template vec2& operator*=(vec2&, const vec2&);

template vec4& operator*=(vec4&, const f32&);
template vec3& operator*=(vec3&, const f32&);
template vec2& operator*=(vec2&, const f32&);

template vec4& operator/=(vec4&, const vec4&);
template vec3& operator/=(vec3&, const vec3&);
template vec2& operator/=(vec2&, const vec2&);

template vec4& operator/=(vec4&, const f32&);
template vec3& operator/=(vec3&, const f32&);
template vec2& operator/=(vec2&, const f32&);
//==============================================================================

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> inv(const vec<T, SIZE>& a)
{
  vec<T, SIZE> one{-1.0f};
  return div(one, a);
}

//==============================================================================
const f32& vec<f32, 4>::operator[](u64 index) const
{
  return (&x)[index];
}

//==============================================================================
const f32& vec<f32, 3>::operator[](u64 index) const
{
  return v[index];
}

//==============================================================================
f32& vec<f32, 4>::operator[](u64 index)
{
  return (&x)[index];
}

//==============================================================================
f32& vec<f32, 3>::operator[](u64 index)
{
  return v[index];
}

//==============================================================================
const f32& vec<f32, 2>::operator[](u64 index) const
{
  return (&x)[index];
}

//==============================================================================
f32& vec<f32, 2>::operator[](u64 index)
{
  return (&x)[index];
}

}

