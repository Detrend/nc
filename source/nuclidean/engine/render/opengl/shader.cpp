// Project Nuclidean Source File
#include <engine/render/opengl/shader.h>

#include <common.h>
#include <logging.h>
#include <metaprogramming.h>

#include <glad/glad.h>

#include <string>
#include <utility>
#include <vector>

namespace nc::render::opengl
{

//==============================================================================
Shader::Shader(cstr compute_source)
  : Shader({{compute_source, GL_COMPUTE_SHADER}}) {}

//==============================================================================
Shader::Shader(cstr vertex_source, cstr fragment_source)
  : Shader({{vertex_source, GL_VERTEX_SHADER}, {fragment_source, GL_FRAGMENT_SHADER}}) {}

//==============================================================================
Shader::Shader(std::initializer_list<std::pair<cstr, GLenum>> shader_sources)
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
  m_handle = *shader_program;
}

//==============================================================================
Shader::~Shader()
{
  if (m_handle)
    glDeleteProgram(m_handle);
}

//==============================================================================
Shader::Shader(Shader&& other) noexcept
  : m_handle(std::exchange(other.m_handle, 0))
{}

//==============================================================================
Shader& Shader::operator=(Shader&& other) noexcept
{
  std::swap(m_handle, other.m_handle);
  return *this;
}

//==============================================================================
void Shader::use() const
{
  glUseProgram(m_handle);
}

//==============================================================================
void Shader::dispatch(size_t group_x, size_t group_y, size_t group_z, bool sync) const
{
  use();
  glDispatchCompute(cast<GLuint>(group_x), cast<GLuint>(group_y), cast<GLuint>(group_z));
  if (sync)
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

//==============================================================================
std::optional<GLuint> Shader::compile_shader(cstr source, GLenum type) const
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

    const std::string message = "[render][shader] compile error: " + std::string(info_log);
    nc_crit("{}", message);
    return std::nullopt;
  }

  return shader;
}

//==============================================================================
std::optional<GLuint> Shader::link_program(std::span<const GLuint> shaders) const
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

    const std::string message = "[render][shader] link error: " + std::string(info_log);
    nc_crit("{}", message);
    return std::nullopt;
  }

  return shader_program;
}

}
