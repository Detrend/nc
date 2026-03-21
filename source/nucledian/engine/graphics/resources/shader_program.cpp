#include <common.h>
#include <logging.h>
#include <math/lingebra.h>
#include <engine/graphics/resources/shader_program.h>

#include <glad/glad.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace nc
{

// ---------------------------------------------------------------------------
// Shader source root – derived once from __FILE__ so the binary always finds
// the source tree regardless of the working directory.
// ---------------------------------------------------------------------------
static std::string g_shader_root;

static std::string compute_default_shader_root()
{
  // __FILE__ is the absolute path to this .cpp file.
  // It lives at: <root>/source/nucledian/engine/graphics/resources/shader_program.cpp
  // Going up 4 levels (strip filename + engine/graphics/resources) reaches <root>/source/nucledian/
  std::string path = __FILE__;
  std::replace(path.begin(), path.end(), '\\', '/');
  for (int i = 0; i < 4; ++i)
  {
    const auto pos = path.rfind('/');
    if (pos != std::string::npos)
      path.resize(pos);
  }
  return path + "/";
}

//==============================================================================
void ShaderProgramHandle::set_shader_root(std::string root)
{
  std::replace(root.begin(), root.end(), '\\', '/');
  if (!root.empty() && root.back() != '/')
    root += '/';
  g_shader_root = std::move(root);
}

//==============================================================================
std::optional<std::string> ShaderProgramHandle::read_shader_file(const char* relative_path)
{
  if (g_shader_root.empty())
    g_shader_root = compute_default_shader_root();

  const std::string full_path = g_shader_root + relative_path;
  std::ifstream file(full_path);
  if (!file.is_open())
  {
    nc_crit("ShaderProgramHandle: cannot open shader file '{}'", full_path);
    return std::nullopt;
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

//==============================================================================
ShaderProgramHandle ShaderProgramHandle::from_files(const char* vert_path, const char* frag_path)
{
  const auto vert = read_shader_file(vert_path);
  const auto frag = read_shader_file(frag_path);
  if (!vert || !frag)
    return ShaderProgramHandle();

  return ShaderProgramHandle(vert->c_str(), frag->c_str());
}

//==============================================================================
ShaderProgramHandle ShaderProgramHandle::from_file(const char* comp_path)
{
  const auto comp = read_shader_file(comp_path);
  if (!comp)
    return ShaderProgramHandle();

  return ShaderProgramHandle(comp->c_str());
}

//==============================================================================
bool ShaderProgramHandle::try_reload(const std::vector<std::string>& file_paths)
{
  std::vector<std::string> sources;
  sources.reserve(file_paths.size());
  for (const auto& path : file_paths)
  {
    const auto src = read_shader_file(path.c_str());
    if (!src)
      return false;
    sources.push_back(*src);
  }

  // Determine shader types from file extensions
  auto get_type = [](const std::string& path) -> GLenum
  {
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".vert") return GL_VERTEX_SHADER;
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".frag") return GL_FRAGMENT_SHADER;
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".comp") return GL_COMPUTE_SHADER;
    return GL_VERTEX_SHADER;
  };

  std::vector<GLuint> shaders;
  shaders.reserve(sources.size());

  auto delete_shaders = [&shaders]()
  {
    for (GLuint s : shaders)
      glDeleteShader(s);
  };

  bool compile_error = false;
  for (size_t i = 0; i < sources.size(); ++i)
  {
    const GLenum type = get_type(file_paths[i]);
    const auto shader = compile_shader(sources[i].c_str(), type, /*log_only=*/true);
    if (!shader)
    {
      compile_error = true;
      continue;
    }
    shaders.push_back(*shader);
  }

  if (compile_error)
  {
    delete_shaders();
    return false;
  }

  const auto new_program = link_program(shaders, /*log_only=*/true);
  delete_shaders();
  if (!new_program)
    return false;

  if (m_shader_program != 0)
    glDeleteProgram(m_shader_program);

  m_shader_program = *new_program;
  return true;
}

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
std::optional<GLuint> ShaderProgramHandle::compile_shader(const char* source, GLenum type, bool log_only) const
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

    const std::string message = "Shader compile error: " + std::string(info_log);
    if (log_only)
      nc_crit("{}", message);
    else
      nc_expect(false, "{0}", message);
    return std::nullopt;
  }

  return shader;
}

//==============================================================================
std::optional<GLuint> ShaderProgramHandle::link_program(std::span<const GLuint> shaders, bool log_only) const
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

    const std::string message = "Shader link error: " + std::string(info_log);
    if (log_only)
      nc_crit("{}", message);
    else
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
