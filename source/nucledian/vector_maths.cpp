// Project Nucledian Source File
#include <vector_maths.h>

namespace nc::sse
{
#include <vector_maths_declarations.h>
}

namespace nc::non_sse
{
#include <vector_maths_declarations.h>
}

namespace nc
{

//==============================================================================
template<typename T, u64 SIZE>
bool eq(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::eq(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> add(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::add(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> sub(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::sub(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> min(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::min(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> max(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::max(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> mul(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::mul(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
vec<T, SIZE> div(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::div(a, b);
}

//==============================================================================
template<typename T, u64 SIZE>
T dot(const vec<T, SIZE>& a, const vec<T, SIZE>& b)
{
  return sse::dot(a, b);
}

//==============================================================================
vec3 cross(const vec3& a, const vec3& b)
{
  return sse::cross(a, b);
}

//==============================================================================
vec4 cross(const vec4& a, const vec4& b)
{
  return sse::cross(a, b);
}

//==============================================================================
f32 cross(const vec2& a, const vec2& b)
{
  return sse::cross(a, b);
}

}

namespace nc
{
#include <vector_maths_instantiations.h>
}

