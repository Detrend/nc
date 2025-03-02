// Project Nucledian Source File
#pragma once

namespace nc
{
  
//==============================================================================
constexpr vec<f32, 4>::vec(vec3 v3, f32 w)
: x(v3.x)
, y(v3.y)
, z(v3.z)
, w(w)
{
  
}

//==============================================================================
constexpr vec<f32, 4>::vec(const sse::reg128& sse_reg)
: r(sse_reg)
{
  
}

//==============================================================================
constexpr vec<f32, 4>::vec(f32 x, f32 y, f32 z, f32 w)
: x(x)
, y(y)
, z(z)
, w(w)
{
  
}

//==============================================================================
constexpr vec<f32, 4>::vec()
: x(0)
, y(0)
, z(0)
, w(0)
{
  
}

//==============================================================================
constexpr vec<f32, 4> vec<f32, 4>::with_x(f32 new_x) const
{
	return vec<f32, 4>(new_x, this->y, this->z, this->w);
}

//==============================================================================
constexpr vec<f32, 4> vec<f32, 4>::with_y(f32 new_y) const
{
	return vec<f32, 4>(this->x, new_y, this->z, this->w);
}

//==============================================================================
constexpr vec<f32, 4> vec<f32, 4>::with_z(f32 new_z) const
{
	return vec<f32, 4>(this->x, this->y, new_z, this->w);
}

//==============================================================================
constexpr vec<f32, 4> vec<f32, 4>::with_w(f32 new_w) const
{
	return vec<f32, 4>(this->x, this->y, this->z, new_w);
}

//==============================================================================
constexpr vec<f32, 3> vec<f32, 4>::truncate() const
{
  return vec<f32, 3>(this->x, this->y, this->z);
}

//==============================================================================
inline constexpr vec<f32, 4> ZERO = vec<f32, 4>(0.0f, 0.0f, 0.0f, 0.0f);
inline constexpr vec<f32, 4> ONE  = vec<f32, 4>(1.0f, 1.0f, 1.0f, 1.0f);
inline constexpr vec<f32, 4> X    = vec<f32, 4>(1.0f, 0.0f, 0.0f, 0.0f);
inline constexpr vec<f32, 4> Y    = vec<f32, 4>(0.0f, 1.0f, 0.0f, 0.0f);
inline constexpr vec<f32, 4> Z    = vec<f32, 4>(0.0f, 0.0f, 1.0f, 0.0f);
inline constexpr vec<f32, 4> W    = vec<f32, 4>(0.0f, 0.0f, 0.0f, 1.0f);

//==============================================================================
constexpr vec<f32, 3>::vec(f32 x, f32 y, f32 z)
: x(x)
, y(y)
, z(z)
{

}

//==============================================================================
constexpr vec<f32, 3>::vec(vec2 v2, f32 z)
: x(v2.x)
, y(v2.y)
, z(z)
{

}

//==============================================================================
constexpr vec<f32, 3>::vec(const vec4& v4)
: x(v4.x)
, y(v4.y)
, z(v4.z)
{
  
}

//==============================================================================
constexpr vec<f32, 3>::vec()
: x(0)
, y(0)
, z(0)
{

}

//==============================================================================
constexpr vec<f32, 3> vec<f32, 3>::with_x(f32 new_x) const
{
	return vec<f32, 3>(new_x, this->y, this->z);
}

//==============================================================================
constexpr vec<f32, 3> vec<f32, 3>::with_y(f32 new_y) const
{
	return vec<f32, 3>(this->x, new_y, this->z);
}

//==============================================================================
constexpr vec<f32, 3> vec<f32, 3>::with_z(f32 new_z) const
{
	return vec<f32, 3>(this->x, this->y, new_z);
}

//==============================================================================
constexpr vec<f32, 2> vec<f32, 3>::truncate() const
{
  return vec<f32, 2>(this->x, this->y);
}

//==============================================================================
inline constexpr vec<f32, 3> vec<f32, 3>::ZERO = vec<f32, 3>(0.0f, 0.0f, 0.0f);
inline constexpr vec<f32, 3> vec<f32, 3>::ONE  = vec<f32, 3>(1.0f, 1.0f, 1.0f);
inline constexpr vec<f32, 3> vec<f32, 3>::X    = vec<f32, 3>(1.0f, 0.0f, 0.0f);
inline constexpr vec<f32, 3> vec<f32, 3>::Y    = vec<f32, 3>(0.0f, 1.0f, 0.0f);
inline constexpr vec<f32, 3> vec<f32, 3>::Z    = vec<f32, 3>(0.0f, 0.0f, 1.0f);

//==============================================================================
constexpr vec<f32, 2>::vec(f32 x, f32 y)
: x(x)
, y(y)
{

}

//==============================================================================
constexpr vec<f32, 2>::vec()
: x(0)
, y(0)
{

}

//==============================================================================
constexpr vec<f32, 2>::vec(const sse::reg64& sse_reg)
: r(sse_reg)
{
  
}

//==============================================================================
constexpr vec<f32, 4>::vec(f32 v)
: x(v)
, y(v)
, z(v)
, w(v)
{
  
}

//==============================================================================
constexpr vec<f32, 3>::vec(f32 v)
: x(v)
, y(v)
, z(v)
{
  
}

//==============================================================================
constexpr vec<f32, 2>::vec(f32 v)
: x(v)
, y(v)
{
  
}

//==============================================================================
constexpr vec<f32, 2> vec<f32, 2>::with_x(f32 new_x) const
{
	return vec<f32, 2>(new_x, this->y);
}

//==============================================================================
constexpr vec<f32, 2> vec<f32, 2>::with_y(f32 new_y) const
{
	return vec<f32, 2>(this->x, new_y);
}

//==============================================================================
inline constexpr vec<f32, 2> vec<f32, 2>::ZERO = vec<f32, 2>(0.0f, 0.0f);
inline constexpr vec<f32, 2> vec<f32, 2>::ONE  = vec<f32, 2>(1.0f, 1.0f);
inline constexpr vec<f32, 2> vec<f32, 2>::X    = vec<f32, 2>(1.0f, 0.0f);
inline constexpr vec<f32, 2> vec<f32, 2>::Y    = vec<f32, 2>(0.0f, 1.0f);

}

