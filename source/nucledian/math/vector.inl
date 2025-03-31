// Project Nucledian Source File
#pragma once

namespace nc
{
  
//==============================================================================
template<typename VT, typename FT>
inline VT with_x(const VT& v, const FT& x)
{
  VT v2 = v;
  v2.x = static_cast<VT::value_type>(x);
  return v2;
}

//==============================================================================
template<typename VT, typename FT>
inline VT with_y(const VT& v, const FT& y)
{
  VT v2 = v;
  v2.y = static_cast<VT::value_type>(y);
  return v2;
}

//==============================================================================
template<typename VT, typename FT>
inline VT with_z(const VT& v, const FT& z)
{
  VT v2 = v;
  v2.z = static_cast<VT::value_type>(z);
  return v2;
}

//==============================================================================
template<typename VT, typename FT>
inline VT with_w(const VT& v, const FT& w)
{
  VT v2 = v;
  v2.w = static_cast<VT::value_type>(w);
  return v2;
}

}

