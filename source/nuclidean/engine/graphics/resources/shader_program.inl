#pragma once

#include <engine/graphics/resources/shader_program.h>

namespace nc
{

//==============================================================================
template<GLint location, typename T>
inline void ShaderProgramHandle::set_uniform(Uniform<location, T>, const T& value) const
{
  this->set_uniform(location, value);
}

}