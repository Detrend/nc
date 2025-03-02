#include <engine/graphics/graphics_system.h>
#include <engine/graphics/gizmo.h>
#include <engine/core/engine.h>

namespace nc
{

//==============================================================================
Gizmo::Gizmo(MeshHandle mesh, const mat4& transform, const color& color, f32 ttl)
  : m_mesh_handle(mesh), m_transform(transform), m_color(color), m_ttl(ttl) {
}

//==============================================================================
MeshHandle Gizmo::get_mesh() const
{
  return m_mesh_handle;
}

//==============================================================================
void Gizmo::set_mesh(MeshHandle mesh_handle)
{
  m_mesh_handle = mesh_handle;
}

//==============================================================================
mat4 Gizmo::get_transform() const
{
  return m_transform;
}

//==============================================================================
void Gizmo::set_transform(const mat4& transform)
{
  m_transform = transform;
}

//==============================================================================
color Gizmo::get_color() const
{
  return m_color;
}

//==============================================================================
void Gizmo::set_color(const color& color)
{
  m_color = color;
}

//==============================================================================
GizmoPtr create_gizmo(MeshHandle mesh_handle, const mat4& transform, const color& color, f32 ttl)
{
  return get_engine().get_module<GraphicsSystem>().create_gizmo(mesh_handle, transform, color, ttl);
}

//==============================================================================
GizmoPtr create_gizmo(MeshHandle mesh_handle, const vec3& position, const color& color, f32 ttl)
{
  return get_engine().get_module<GraphicsSystem>().create_gizmo(mesh_handle, position, color, ttl);
}

}
