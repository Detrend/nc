#pragma once

#include <temp_math.h>
#include <engine/graphics/resources/mesh.h>
#include <engine/graphics/resources/material.h>

#include <glad/glad.h>

#include <memory>
#include <unordered_map>

namespace nc
{

/**
 * Represents a collection of rendering assets needed to render an object.
 */
struct Model
{
  // Creates an invalid model.
  Model() {}
  Model(MeshHandle mesh);

  MeshHandle mesh = MeshHandle::invalid();
  MaterialHandle material = MaterialHandle::invalid();
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

  MeshHandle get_mesh() const;
  void set_mesh(MeshHandle mesh_handle);

  mat4 get_transform() const;
  void set_transform(const mat4& transform);

  color get_color() const;
  void set_color(const color& color);

private:
  // Constructor is private so its callable only from Renderer class.
  Gizmo(MeshHandle mesh_handle, const mat4& transform, const color& color, f32 ttl);

  MeshHandle m_mesh_handle = MeshHandle::invalid();
  mat4       m_transform   = mat4(1.0f);
  color      m_color       = colors::WHITE;
  f32        m_ttl         = 0.0f;
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
  void init();
  /**
   * Renders the complete scene including:
   * - Entities with RenderComponent
   * - Level sectors (rooms)
   * - Gizmos
   */
  void render() const;
  /**
   * Update time to live of gizmos and delete gizmos with ttl less than zero.
   */
  void update_gizmos(f32 delta_seconds);

  /**
  * Creates a new gizmo.
  *
  * The created gizmo will remain visible until:
  * - All GizmoPtr references are destroyed, OR
  * - Time to live (ttl) expires (if specified) [seconds]
  */
  GizmoPtr create_gizmo(MeshHandle mesh_handle, const mat4& transform, const color& color, f32 ttl = 0.0f);
  /**
  * Creates a new gizmo.
  *
  * The created gizmo will remain visible until:
  * - All GizmoPtr references are destroyed, OR
  * - Time to live (ttl) expires (if specified) [seconds]
  */
  GizmoPtr create_gizmo(MeshHandle mesh_handle, const vec3& position, const color& color, f32 ttl = 0.0f);

private:
  using GizmoMap = std::unordered_map<u64, Gizmo>;
  
  void render_gizmos() const;

  u64            m_next_gizmo_id         = 0;
  // Material used for rendering gizmos.
  MaterialHandle m_gizmo_material_handle = MaterialHandle::invalid();
  GizmoMap       m_gizmos;

  // Temporary gizmo 1.
  GizmoPtr       m_temp_cube_gizmo1;
  // Temporary gizmo 2.
  GizmoPtr       m_temp_cube_gizmo2;
};

}