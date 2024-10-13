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
vec<F, S> operator*(const vec<F, S>& a, const vec<F, S>& b)
{
  return mul(a, b);
}

//==============================================================================
template<typename F, u64 S>
vec<F, S> operator/(const vec<F, S>& a, const vec<F, S>& b)
{
  return div(a, b);
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

template vec4 operator/(const vec4&, const vec4&);
template vec3 operator/(const vec3&, const vec3&);
template vec2 operator/(const vec2&, const vec2&);

template vec4 operator-(const vec4&);
template vec3 operator-(const vec3&);
template vec2 operator-(const vec2&);
//==============================================================================

//==============================================================================
vec<f32, 4>::operator sse::reg128() const
{
  return r;
}

//==============================================================================
vec<f32, 4>::operator sse::reg128& ()
{
  return r;
}

//==============================================================================
vec<f32, 2>::operator sse::reg64() const
{
  return r;
}

//==============================================================================
vec<f32, 2>::operator sse::reg64& ()
{
  return r;
}

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
  return v[index];
}

//==============================================================================
const f32& vec<f32, 3>::operator[](u64 index) const
{
  return v[index];
}

//==============================================================================
const f32& vec<f32, 2>::operator[](u64 index) const
{
  return v[index];
}

//==============================================================================
f32& vec<f32, 4>::operator[](u64 index)
{
  return v[index];
}

//==============================================================================
f32& vec<f32, 3>::operator[](u64 index)
{
  return v[index];
}

//==============================================================================
f32& vec<f32, 2>::operator[](u64 index)
{
  return v[index];
}

}

