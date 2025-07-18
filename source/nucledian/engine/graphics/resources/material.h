#pragma once

#include <math/vector.h>
#include <engine/graphics/shaders/shaders.h>
#include <engine/graphics/gl_types.h>

#include <optional>

namespace nc
{

// Light-weight shader handle.
struct MaterialHandle
{
public:
  friend class GizmoManager;
  friend class GraphicsSystem;
  friend class Renderer;

  MaterialHandle(const char* vertex_source, const char* fragment_source);

  bool is_valid() const;
  operator bool() const;

  // Gets a invalid material.
  static MaterialHandle invalid();

  void use() const;

// Shaders should be manipulated only within render system and related classes.
private:
  MaterialHandle() {}

  std::optional<GLuint> compile_shader(const char* source, GLenum type) const;
  std::optional<GLuint> link_program(GLuint vertex_shader, GLuint fragment_shader) const;

  template<GLint location, typename T>
  void set_uniform(Uniform<location, T> uniform, const T& value) const;

  void set_uniform(GLint location, const mat4& value) const;
  void set_uniform(GLint location, const vec3& value) const;
  void set_uniform(GLint location, const vec4& value) const;
  void set_uniform(GLint location, s32 value)         const;
  void set_uniform(GLint location, f32 value)         const;

  GLuint m_shader_program = 0;
};

}

#include <engine/graphics/resources/material.inl>
