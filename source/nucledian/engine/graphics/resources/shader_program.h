#pragma once

#include <engine/graphics/uniform.h>
#include <engine/graphics/gl_types.h>

#include <math/vector.h>
#include <math/matrix.h>

#include <initializer_list>
#include <optional>
#include <utility>
#include <span>

namespace nc
{

// Light-weight shader handle.
struct ShaderProgramHandle
{
public:
  friend class GizmoManager;
  friend class GraphicsSystem;
  friend class Renderer;
  friend class UiButton;
  friend class UiHudDisplay;

  explicit ShaderProgramHandle(const char* compute_source);
  ShaderProgramHandle(const char* vertex_source, const char* fragment_source);

  bool is_valid() const;
  operator bool() const;

  // Gets a invalid material.
  static ShaderProgramHandle invalid();

  void use() const;
  void dispatch(size_t group_x, size_t group_y, size_t group_z, bool sync) const;

// Shaders should be manipulated only within render system and related classes.
private:
  ShaderProgramHandle() {}
  explicit ShaderProgramHandle(std::initializer_list<std::pair<const char*, GLenum>> shader_sources);

  std::optional<GLuint> compile_shader(const char* source, GLenum type) const;
  std::optional<GLuint> link_program(std::span<const GLuint> shaders) const;

  template<GLint location, typename T>
  void set_uniform(Uniform<location, T> uniform, const T& value) const;

  void set_uniform(GLint location, const mat4& value) const;
  void set_uniform(GLint location, const vec2& value) const;
  void set_uniform(GLint location, const vec3& value) const;
  void set_uniform(GLint location, const vec4& value) const;
  void set_uniform(GLint location, u32 value)         const;
  void set_uniform(GLint location, s32 value)         const;
  void set_uniform(GLint location, f32 value)         const;

  GLuint m_shader_program = 0;
};

}

#include <engine/graphics/resources/shader_program.inl>
