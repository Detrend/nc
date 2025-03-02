#include "renderer.h"

#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/shaders/shaders.h>

#include <iterator>

namespace nc
{

//==============================================================================
Model::Model(MeshHandle mesh)
  : mesh(mesh) {}

//==============================================================================
Gizmo::Gizmo(MeshHandle mesh, const mat4& transform, const color& color, f32 ttl)
  : m_mesh_handle(mesh), m_transform(transform), m_color(color), m_ttl(ttl) {}

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
void Renderer::init()
{
  // TODO: temporary cube gizmo
  m_temp_cube_gizmo1 = create_gizmo(meshes::cube(),  vec3::X, colors::RED, 5.0f);
  m_temp_cube_gizmo2 = create_gizmo(meshes::cube(), -vec3::X, colors::BLUE, 9.0f);

  m_gizmo_material_handle = MaterialManager::instance()->create
  (
    shaders::gizmo::NAME,
    shaders::gizmo::VERTEX_SOURCE,
    shaders::gizmo::FRAGMENT_SOURCE,
    ResourceLifetime::Game
  );

  // setup projection matrix
  const mat4 projection = perspective(radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
  set_uniform(m_gizmo_material_handle, shaders::gizmo::PROJECTION, projection);
}

//==============================================================================
void Renderer::render() const
{
  // TODO: indirect rendering

  // TODO: render entities
  // TODO: render secors

  render_gizmos();
}

//==============================================================================
void Renderer::update_gizmos(f32 delta_seconds)
{
  std::vector<u64> to_delete;

  for (auto& [id, gizmo] : m_gizmos)
  {
    gizmo.m_ttl -= delta_seconds;
    if (gizmo.m_ttl < 0.0f)
    {
      to_delete.push_back(id);
    }
  }

  for (auto& id : to_delete)
  {
    m_gizmos.erase(id);
  }
}

//==============================================================================
GizmoPtr Renderer::create_gizmo(MeshHandle mesh_handle, const mat4& transform, const color& color, f32 ttl)
{
  u64 id = m_next_gizmo_id++;
  auto [it, _] = m_gizmos.emplace(id, Gizmo(mesh_handle, transform, color, ttl));

  auto deleter = [this, id](const Gizmo*)
  {
    auto it = m_gizmos.find(id);

    if (it != m_gizmos.end())
    {
      m_gizmos.erase(it);
    }
  };

  return GizmoPtr(&it->second, deleter);
}

//==============================================================================
GizmoPtr Renderer::create_gizmo(MeshHandle mesh_handle, const vec3& position, const color& color, f32 ttl)
{
  return create_gizmo(mesh_handle, translate(mat4(1.0f), position), color, ttl);
}

//==============================================================================
void Renderer::render_gizmos() const
{
  // TODO: render only visible gizmos

  const MeshManager* meshes = MeshManager::instance();
  use_material(m_gizmo_material_handle);

  const mat4 view = get_engine().get_module<GraphicsSystem>().get_debug_camera().get_view();
  set_uniform(m_gizmo_material_handle, shaders::gizmo::VIEW, view);

  for (const auto& [_, gizmo] : m_gizmos)
  {
    const Mesh& mesh = meshes->get_resource(gizmo.m_mesh_handle);

    set_uniform(m_gizmo_material_handle, shaders::gizmo::TRANSFORM, gizmo.get_transform());
    set_uniform(m_gizmo_material_handle, shaders::gizmo::COLOR, gizmo.get_color().truncate());

    glBindVertexArray(mesh.get_vao());
    glDrawArrays(GL_TRIANGLES, 0, mesh.get_vertex_count());
  }
}

}