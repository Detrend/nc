#include <engine/graphics/resources/material.h>

#include <stdexcept>

namespace nc
{

//==============================================================================
Material::Material(const std::string_view& shader_name, GLuint shader_program)
  : m_shader_program(shader_program), m_shader_name(shader_name) {
}


//==============================================================================
void Material::use() const
{
  if (m_current_program == m_shader_program)
  {
    return;
  }

  glUseProgram(m_shader_program);
  m_current_program = m_shader_program;
}

//==============================================================================
void Material::unload()
{
  glDeleteProgram(m_shader_program);
  m_shader_program = 0;
}

//==============================================================================
void Material::set_uniform(GLint location, const mat4& value) const
{
  glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(value));
}

//==============================================================================
void Material::set_uniform(GLint location, const vec3& value) const
{
  glUniform3f(location, value.x, value.y, value.z);
}

void Material::set_uniform(GLint location, const vec4& value) const
{
  glUniform4f(location, value.x, value.y, value.z, value.w);
}

//==============================================================================
MaterialManager* MaterialManager::instance()
{
  static std::unique_ptr<MaterialManager> instance(new MaterialManager());
  return instance.get();
}

//==============================================================================
MaterialHandle MaterialManager::create
(
  const std::string_view& shader_name,
  const char* vertex_source,
  const char* fragment_source,
  ResourceLifetime lifetime
)
{
  GLint success = 0;

  // compile vertex shader
  const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_source, nullptr);
  glCompileShader(vertex_shader);

  // check vertex shader compile status
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    GLchar info_log[512];
    glGetShaderInfoLog(vertex_shader, sizeof(info_log), nullptr, info_log);
    glDeleteShader(vertex_shader);

    throw std::runtime_error
    (
      "Vertex shader (" + std::string(shader_name) + ") compilaion failed : " + std::string(info_log)
    );
  }

  // compile fragment shader
  const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_source, nullptr);
  glCompileShader(fragment_shader);

  // check fragment shader compile status
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    GLchar info_log[512];
    glGetShaderInfoLog(vertex_shader, sizeof(info_log), nullptr, info_log);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    throw std::runtime_error
    (
      "Fragment shader (" + std::string(shader_name) + ") compilaion failed : " + std::string(info_log)
    );
  }

  // link program
  const GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);

  // check program link status
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success)
  {
    GLchar info_log[512];
    glGetProgramInfoLog(vertex_shader, sizeof(info_log), nullptr, info_log);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    glDeleteProgram(shader_program);

    throw std::runtime_error
    (
      "Shader program (" + std::string(shader_name) + ") linking failed : " + std::string(info_log)
    );
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return this->register_resource(Material(shader_name, shader_program), lifetime);
}

//==============================================================================
void use_material(MaterialHandle handle)
{
  MaterialManager::instance()->get_resource(handle).use();
}

}
