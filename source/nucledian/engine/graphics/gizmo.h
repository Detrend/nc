#pragma once

#include <engine/graphics/resources/mesh.h>

#include <temp_math.h>
#include <memory>

namespace nc
{

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

}
