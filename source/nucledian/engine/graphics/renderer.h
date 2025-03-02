#pragma once

#include <engine/graphics/resources/mesh.h>
#include <engine/graphics/resources/material.h>
#include <engine/graphics/gizmo.h>

#include <glad/glad.h>

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