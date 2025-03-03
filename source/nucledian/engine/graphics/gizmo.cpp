#include <engine/graphics/gizmo.h>

#include <engine/core/engine.h>
#include <engine/graphics/shaders/shaders.h>
#include <engine/graphics/graphics_system.h>

#include <bit>
#include <ranges>
#include <array>

namespace nc
{

//==============================================================================
GizmoPtr Gizmo::create_cube(const vec3& pos, f32 size, const color& color)
{
  auto& system = get_engine().get_module<GraphicsSystem>();
  auto& gizmo_manager = system.get_gizmo_manager();
  const u32 id = gizmo_manager.m_next_gizmo_id++;

  auto[it, _] = gizmo_manager.m_gizmos.emplace
  (
    id,
    Gizmo(system.get_mesh_manager().get_cube(), pos, size, color, 0.0f)
  );

  auto deleter = [&gizmo_manager, id](const Gizmo*)
  {
    auto it = gizmo_manager.m_gizmos.find(id);
    if (it == gizmo_manager.m_gizmos.end())
    {
      return;
    }

    gizmo_manager.m_gizmos.erase(it);
  };

  return GizmoPtr(&(it->second), deleter);
}

//==============================================================================
void Gizmo::create_cube(f32 ttl, const vec3& pos, f32 size, const color& color)
{
  auto& system = get_engine().get_module<GraphicsSystem>();
  auto& gizmo_manager = system.get_gizmo_manager();
  const u32 id = gizmo_manager.m_next_gizmo_id++;

  gizmo_manager.m_ttl_gizmos.emplace(id, Gizmo(system.get_mesh_manager().get_cube(), pos, size, color, ttl));
}

//==============================================================================
Gizmo::Gizmo(const Mesh& mesh, const vec3& pos, f32 size, const color& color, f32 ttl)
  : m_ttl(ttl), m_mesh(mesh), m_color(color), m_transform(scale(translate(mat4(1.0f), pos), vec3(size))) {}

//==============================================================================
void GizmoManager::init()
{
  m_gizmo_material = Material(shaders::gizmo::VERTEX_SOURCE, shaders::gizmo::FRAGMENT_SOURCE);

  const mat4 projection = perspective(radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
  m_gizmo_material.use();
  m_gizmo_material.set_uniform(shaders::gizmo::PROJECTION, projection);
}

//==============================================================================
void GizmoManager::update_ttls(f32 delta_seconds)
{
  std::vector<u32> to_remove;
  for (auto& [id, gizmo] : m_ttl_gizmos)
  {
    gizmo.m_ttl -= delta_seconds;

    if (gizmo.m_ttl < 0.0f)
    {
      to_remove.push_back(id);
    }
  }

  for (const auto& id : to_remove)
  {
    m_ttl_gizmos.erase(id);
  }
}

//==============================================================================
void GizmoManager::draw_gizmos() const
{
  m_gizmo_material.use();

  const mat4 view = get_engine().get_module<GraphicsSystem>().get_debug_camera().get_view();
  m_gizmo_material.set_uniform(shaders::gizmo::VIEW, view);

  auto combined_view = std::ranges::views::join
  (
    std::array{ std::views::all(m_gizmos), std::views::all(m_ttl_gizmos) }
  );

  for (const auto& [_, gizmo] : combined_view)
  {
    m_gizmo_material.set_uniform(shaders::gizmo::TRANSFORM, gizmo.m_transform);
    m_gizmo_material.set_uniform(shaders::gizmo::COLOR, gizmo.m_color);

    glBindVertexArray(gizmo.m_mesh.get_vao());
    glDrawArrays(gizmo.m_mesh.get_draw_mode(), 0, gizmo.m_mesh.get_vertex_count());
  }
}

}
