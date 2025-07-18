#include <common.h>
#include <math/lingebra.h>
#include <engine/graphics/resources/material.h>

#include <iostream>

namespace nc
{

//==============================================================================
MaterialHandle::MaterialHandle(const char* vertex_source, const char* fragment_source)
{
  const std::optional<GLuint> vertex_shader = compile_shader(vertex_source, GL_VERTEX_SHADER);
  if (!vertex_shader)
  {
    return;
  }

  const std::optional<GLuint> fragment_shader = compile_shader(fragment_source, GL_FRAGMENT_SHADER);
  if (!fragment_shader)
  {
    glDeleteShader(*vertex_shader);
    return;
  }

  const std::optional<GLuint> shader_program = link_program(*vertex_shader, *fragment_shader);
  if (!shader_program)
  {
    glDeleteShader(*vertex_shader);
    glDeleteShader(*fragment_shader);
    return;
  }

  glDeleteShader(*vertex_shader);
  glDeleteShader(*fragment_shader);

  this->m_shader_program = *shader_program;
}

//==============================================================================
bool MaterialHandle::is_valid() const
{
  return m_shader_program != 0;
}

//==============================================================================
MaterialHandle::operator bool() const
{
  return this->is_valid();
}

//==============================================================================
MaterialHandle MaterialHandle::invalid()
{
    return MaterialHandle();
}

//==============================================================================
std::optional<GLuint> MaterialHandle::compile_shader(const char* source, GLenum type) const
{
  const GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  GLint success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    GLchar info_log[512];
    glGetShaderInfoLog(shader, sizeof(info_log), nullptr, info_log);
    glDeleteShader(shader);

    std::string message =  "Shader compile error: " + std::string(info_log);
    nc_expect(false, "{0}", message);
    return std::nullopt;
  }

  return shader;
}

//==============================================================================
std::optional<GLuint> MaterialHandle::link_program(GLuint vertex_shader, GLuint fragment_shader) const
{
  const GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);

  GLint success = 0;
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success)
  {
    GLchar info_log[512];
    glGetProgramInfoLog(vertex_shader, sizeof(info_log), nullptr, info_log);
    glDeleteProgram(shader_program);

    std::string message =  "Shader link error: " + std::string(info_log);
    nc_expect(false, "{0}", message);
    return std::nullopt;
  }

  return shader_program;
}

//==============================================================================
void MaterialHandle::use() const
{
  glUseProgram(m_shader_program);
}

//==============================================================================
void MaterialHandle::set_uniform(GLint location, const mat4& value) const
{
  glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(value));
}

//==============================================================================
void MaterialHandle::set_uniform(GLint location, const vec3& value) const
{
  glUniform3f(location, value.x, value.y, value.z);
}

//==============================================================================
void MaterialHandle::set_uniform(GLint location, const vec4& value) const
{
  glUniform4f(location, value.x, value.y, value.z, value.w);
}

//==============================================================================
void MaterialHandle::set_uniform(GLint location, s32 value) const
{
  glUniform1i(location, value);
}

//==============================================================================
void MaterialHandle::set_uniform(GLint location, f32 value) const
{
  glUniform1f(location, value);
}

}