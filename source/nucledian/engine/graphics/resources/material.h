#pragma once

#include <temp_math.h>
#include <engine/graphics/shaders/shaders.h>

#include <glad/glad.h>

#include <optional>

namespace nc
{

// Light-weight shader handle.
struct Material
{
public:
  friend class GizmoManager;
  friend class GraphicsSystem;

  Material(const char* vertex_source, const char* fragment_source);

  // Gets a invalid material.
  static Material invalid();

// Shaders should be manipulated only within render system and related classes.
private:
  Material() {}

  std::optional<GLuint> compile_shader(const char* source, GLenum type) const;
  std::optional<GLuint> link_program(GLuint vertex_shader, GLuint fragment_shader) const;

  void use() const;

  template<GLint location, typename T>
  void set_uniform(Uniform<location, T> uniform, const T& value) const;

  void set_uniform(GLint location, const mat4& value) const;
  void set_uniform(GLint location, const vec3& value) const;
  void set_uniform(GLint location, const vec4& value) const;
  void set_uniform(GLint location, f32 value)         const;

  GLuint m_shader_program = 0;
};

}

#include <engine/graphics/resources/material.inl>
