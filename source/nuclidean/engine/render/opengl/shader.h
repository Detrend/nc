// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <engine/render/opengl/gl_types.h>

#include <optional>
#include <span>
#include <utility>

namespace nc::render::opengl
{

class Shader
{
public:
  Shader() = default;
  explicit Shader(cstr compute_source);
  Shader(cstr vertex_source, cstr fragment_source);
  ~Shader();

  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;
  Shader(Shader&& other) noexcept;
  Shader& operator=(Shader&& other) noexcept;

  void use() const;
  void dispatch(size_t group_x, size_t group_y, size_t group_z, bool sync) const;

private:
  GLuint m_handle = 0;

  explicit Shader(std::initializer_list<std::pair<cstr, GLenum>> shader_sources);

  std::optional<GLuint> compile_shader(cstr source, GLenum type) const;
  std::optional<GLuint> link_program(std::span<const GLuint> shaders) const;

};

}
