#pragma once

#include <engine/core/resources.h>
#include <engine/graphics/shaders/shaders.h>

#include <glad/glad.h>

#include <string_view>

namespace nc
{

/**
* Class for manupulation with a shader.
*
* A material is typically created and managed by the `ResourceManager<Material>`. Direct interaction with the
* `Material` class is ussually unnecessary; instead, it is recommended to use `ResourceHandle<Material>`.
*/
class Material
{
public:
  Material(const std::string_view& shader_name, GLuint shader_program);

  void use() const;

  /**
   * Set uniform variable to specified value. To specify which uniform to set use predefined constant in
   * shaders::<shader_name>::<uniform_name> (header: shaders.h).
   * 
   * For example to set a "color" uniform of a "gizmo" shder to red color:
   *     material.set_uniform(shaders::gizmo::COLOR, colors::RED);
   */
  template<const std::string_view& shader_name, GLint location, typename T>
  void set_uniform(Uniform<shader_name, location, T> uniform, const T& value) const;

private:
  inline static GLuint m_current_program = 0;

  GLuint m_shader_program;
  const std::string_view m_shader_name;

  void unload();

  void set_uniform(GLint location, const mat4& value) const;
  void set_uniform(GLint location, const vec3& value) const;
  void set_uniform(GLint location, const vec4& value) const;
};
using MaterialHandle = ResourceHandle<Material>;

class MaterialManager : public ResourceManager<Material>
{
public:
  // TODO: cancel singleton pattern, move instance probably to GraphicsSystem
  static MaterialManager* instance();

  MaterialHandle create
  (
    const std::string_view& shader_name,
    const char* vertex_source,
    const char* fragment_source,
    ResourceLifetime lifetime
  );
};

/**
  * Set uniform variable to specified value. To specify which uniform to set use predefined constant in
  * shaders::<shader_name>::<uniform_name> (header: shaders.h).
  *
  * For example to set a "color" uniform of a "gizmo" shder to red color:
  *     set_uniform(handle, shaders::gizmo::COLOR, colors::RED);
  */
template<const std::string_view& shader_name, GLint location, typename T>
void set_uniform(MaterialHandle handle, Uniform<shader_name, location, T> uniform, const T& value);

void use_material(MaterialHandle handle);

}

#include <engine/graphics/resources/material.inl>
