#pragma once

#include <engine/graphics/render_resources.h>
#include <temp_math.h>

#include <glad/glad.h>

#include <memory>
#include <list>

namespace nc
{

/**
 * Represents a collection of rendering assets needed to render an object.
 */
struct Model
{
  // Creates an invalid model.
  Model() {}
  Model(ResourceHandle<Mesh> mesh);

  ResourceHandle<Mesh> mesh = ResourceHandle<Mesh>::invalid();
};

/**
 * Component that enables an entity to be rendered in the scene.
 */
struct RenderComponent
{
  Model model;
  // TODO: transform
};

/**
 * Temporary visual marker for debugging. Gizmos are light-weight render objects that can be created and destroyed at
 * runtime.
 * 
 * Gizmo instances should only be created through Renderer::create_gizmo or GraphicsSystem::create_gizmo. Lifetime of
 * gizmos is managed automatically by GizmoPtr and Renderer.
 */
class Gizmo
{
public:
  friend class Renderer;

  // Creates an invalid gizmo.
  Gizmo() {}

  ResourceHandle<Mesh> get_mesh() const;
  void set_mesh(ResourceHandle<Mesh> mesh_handle);

private:
  // Constructor is private so its callable only from Renderer class.
  Gizmo(ResourceHandle<Mesh> mesh_handle);

  ResourceHandle<Mesh> m_mesh_handle = ResourceHandle<Mesh>::invalid();
  // TODO: transform
  // TODO: color modulation
};
/**
 * Smart pointer managing the lifetime of a Gizmo instance.
 * The referenced gizmo will be automatically destroyed when:
 * - All owning GizmoPtr instances are destroyed, OR
 * - Specified time-to-live duration expires (when used)
 */
using GizmoPtr = std::shared_ptr<Gizmo>;

/**
 * Handles rendering of objects in the scene.
 */
class Renderer
{
public:
  ~Renderer();

  void init();
  /**
   * Renders the complete scene including:
   * - Entities with RenderComponent
   * - Level sectors (rooms)
   * - Gizmos
   */
  void render() const;

  // TODO: time to live
  /**
  * Creates a new gizmo.
  *
  * The created gizmo will remain visible until:
  * - All GizmoPtr references are destroyed, OR
  * - Time to live (ttl) expires (if specified) (ttl is future feature and is not currently implemented)
  */
  GizmoPtr create_gizmo(ResourceHandle<Mesh> mesh);

private:
  void render_gizmos() const;

  std::list<Gizmo> m_gizmos;
  GLuint m_gizmos_shader_program = 0;
  GizmoPtr m_temp_cube_gizmo;

};

}