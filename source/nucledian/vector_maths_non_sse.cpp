// Project Nucledian Source File
#include <vec.h>

namespace nc::non_sse
{
#include <vector_maths_declarations.h>
}

namespace nc::non_sse
{

//==============================================================================
template<typename T, u64 SIZE>
bool eq(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  for (u64 i = 0; i < SIZE; ++i)
  {
    if (a[i] != b[i])
    {
      return false;
    }
  }

  return true;
}
  
//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> min(const vec<T, SIZE>& v1, const vec<T, SIZE>& v2)
{
  vec<T, SIZE> result;
  for (u64 i = 0; i < SIZE; ++i)
  {
    result[i] = v1[i] < v2[i] ? v1[i] : v2[i];
  }
  return result;
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> max(const vec<T, SIZE>& v1, const vec<T, SIZE>& v2)
{
  vec<T, SIZE> result;
  for (u64 i = 0; i < SIZE; ++i)
  {
    result[i] = v1[i] > v2[i] ? v1[i] : v2[i];
  }
  return result;
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> add(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  vec<T, SIZE> result;
  for (u64 i = 0; i < SIZE; ++i)
  {
    result[i] = a[i]+b[i];
  }
  return result;
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> mul(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  vec<T, SIZE> result;
  for (u64 i = 0; i < SIZE; ++i)
  {
    result[i] = a[i]*b[i];
  }
  return result;
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> sub(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  vec<T, SIZE> result;
  for (u64 i = 0; i < SIZE; ++i)
  {
    result[i] = a[i]-b[i];
  }
  return result;
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> div(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  vec<T, SIZE> result;
  for (u64 i = 0; i < SIZE; ++i)
  {
    result[i] = a[i]/b[i];
  }
  return result;
}

//==============================================================================
template<typename T, u64 SIZE>
T dot(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  T result = 0;
  for (u64 i = 0; i < SIZE; ++i)
  {
    result += a[i]*b[i];
  }
  return result;
}

//==============================================================================
f32 cross(const vec2& a, const vec2& b)
{
  return a.x * b.y - a.y * b.x;
}

//==============================================================================
//vec3 cross(const vec3& a, const vec3& b)
//{
//  return vec3{
//    a.y * b.z - a.z * b.y,
//    a.z * b.x - a.x * b.z,
//    a.x * b.y - a.y * b.x};
//}

//==============================================================================
vec4 cross(const vec4& a, const vec4& b)
{
  return vec4{
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x,
    0.0f};
}

}

namespace nc::non_sse
{

#include <vector_maths_instantiations.h>

}

