#include <common.h>
#include <math/lingebra.h>
#include <engine/graphics/resources/shader_program.h>

#include <glad/glad.h>

#include <iostream>
#include <vector>

namespace nc
{

//==============================================================================
ShaderProgramHandle::ShaderProgramHandle(const char* compute_source)
  : ShaderProgramHandle({{compute_source, GL_COMPUTE_SHADER}}) {}

//==============================================================================
ShaderProgramHandle::ShaderProgramHandle(const char* vertex_source, const char* fragment_source)
  : ShaderProgramHandle({{vertex_source, GL_VERTEX_SHADER}, {fragment_source, GL_FRAGMENT_SHADER}}) {}

//==============================================================================
bool ShaderProgramHandle::is_valid() const
{
  return m_shader_program != 0;
}

//==============================================================================
ShaderProgramHandle::operator bool() const
{
  return this->is_valid();
}

//==============================================================================
ShaderProgramHandle ShaderProgramHandle::invalid()
{
    return ShaderProgramHandle();
}

//==============================================================================
ShaderProgramHandle::ShaderProgramHandle(std::initializer_list<std::pair<const char*, GLenum>> shader_sources)
{
  std::vector<GLuint> shaders;
  shaders.reserve(shader_sources.size());

  auto delete_shaders = [&shaders]()
  {
    for (auto&& shader : shaders)
    {
      glDeleteShader(shader);
    }
  };

  // compile shader sources
  bool compile_error = false;
  for (auto&& [source, type] : shader_sources)
  {
    const std::optional<GLuint> shader = compile_shader(source, type);

    if (!shader)
    {
      compile_error = true;
      continue;
    }

    shaders.push_back(*shader);
  }

  // clean up on compile error
  if (compile_error)
  {
    delete_shaders();
    return;
  }

  // create and link shader program
  const std::optional<GLuint> shader_program = link_program(shaders);
  if (!shader_program)
  {
    delete_shaders();
    return;
  }

  delete_shaders();
  m_shader_program = *shader_program;
}

//==============================================================================
std::optional<GLuint> ShaderProgramHandle::compile_shader(const char* source, GLenum type) const
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
std::optional<GLuint> ShaderProgramHandle::link_program(std::span<const GLuint> shaders) const
{
  const GLuint shader_program = glCreateProgram();
  for (GLuint shader : shaders)
  {
    glAttachShader(shader_program, shader);
  }
  glLinkProgram(shader_program);

  GLint success = 0;
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success)
  {
    GLchar info_log[512];
    glGetProgramInfoLog(shader_program, sizeof(info_log), nullptr, info_log);
    glDeleteProgram(shader_program);

    std::string message = "Shader link error: " + std::string(info_log);
    nc_expect(false, "{0}", message);
    return std::nullopt;
  }

  return shader_program;
}

//==============================================================================
void ShaderProgramHandle::use() const
{
  glUseProgram(m_shader_program);
}

//==============================================================================
void ShaderProgramHandle::dispatch(size_t group_x, size_t group_y, size_t group_z, bool sync) const
{
  glDispatchCompute(static_cast<GLuint>(group_x), static_cast<GLuint>(group_y), static_cast<GLuint>(group_z));

  if (sync)
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//==============================================================================
void ShaderProgramHandle::set_uniform(GLint location, const mat4& value) const
{
  glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(value));
}

//==============================================================================
void ShaderProgramHandle::set_uniform(GLint location, const vec2& value) const
{
  glUniform2f(location, value.x, value.y);
}

//==============================================================================
void ShaderProgramHandle::set_uniform(GLint location, const vec3& value) const
{
  glUniform3f(location, value.x, value.y, value.z);
}

//==============================================================================
void ShaderProgramHandle::set_uniform(GLint location, const vec4& value) const
{
  glUniform4f(location, value.x, value.y, value.z, value.w);
}

//==============================================================================
void ShaderProgramHandle::set_uniform(GLint location, u32 value) const
{
  glUniform1ui(location, value);
}

//==============================================================================
void ShaderProgramHandle::set_uniform(GLint location, s32 value) const
{
  glUniform1i(location, value);
}

//==============================================================================
void ShaderProgramHandle::set_uniform(GLint location, f32 value) const
{
  glUniform1f(location, value);
}

}