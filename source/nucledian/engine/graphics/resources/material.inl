#pragma once

#include <engine/graphics/resources/material.h>
#include <engine/graphics/shaders/shaders.h>

#include <glad/glad.h>

#include <cassert>
#include <string_view>

namespace nc
{

//==============================================================================
template<const std::string_view& shader_name, GLint location, typename T>
inline void Material::set_uniform(Uniform<shader_name, location, T>, const T& value) const
{
  assert(m_shader_name == shader_name && "Used uniform is for different shader.");
  this->use();
  this->set_uniform(location, value);
}

//==============================================================================
template<const std::string_view& shader_name, GLint location, typename T>
void set_uniform(MaterialHandle handle, Uniform<shader_name, location, T> uniform, const T& value)
{
  MaterialManager::instance()->get_resource(handle).set_uniform(uniform, value);
}

}