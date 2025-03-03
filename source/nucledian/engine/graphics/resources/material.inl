#pragma once

#include <engine/graphics/resources/material.h>

namespace nc
{

//==============================================================================
template<GLint location, typename T>
inline void Material::set_uniform(Uniform<location, T>, const T& value) const
{
  this->set_uniform(location, value);
}

}