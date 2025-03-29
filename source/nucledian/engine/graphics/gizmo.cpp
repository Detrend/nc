#include <engine/graphics/gizmo.h>

#ifdef NC_DEBUG_DRAW

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
  auto& graphics_system = get_engine().get_module<GraphicsSystem>();
  const Material& solid_material = graphics_system.get_solid_material();

  solid_material.use();

  const DebugCamera& camera =graphics_system .get_debug_camera();
  solid_material.set_uniform(shaders::solid::VIEW, camera.get_view());
  solid_material.set_uniform(shaders::solid::VIEW_POSITION, camera.get_position());

  auto combined_view = std::ranges::views::join
  (
    std::array{ std::views::all(m_gizmos), std::views::all(m_ttl_gizmos) }
  );

  for (const auto& [_, gizmo] : combined_view)
  {
    solid_material.set_uniform(shaders::solid::TRANSFORM, gizmo.m_transform);
    solid_material.set_uniform(shaders::solid::COLOR, gizmo.m_color);

    glBindVertexArray(gizmo.m_mesh.get_vao());
    glDrawArrays(gizmo.m_mesh.get_draw_mode(), 0, gizmo.m_mesh.get_vertex_count());
  }
}

//==============================================================================
void GizmoManager::draw_line(
  const vec2& from,
  const vec2& to,
  const vec3& col)
{
  m_primitives.push_back(Primitive
  {
  .type = PrimitiveType::LineType,
  .line = Line
  {
    .from  = from,
    .to    = to,
    .color = col,
  }});
}

//==============================================================================
void GizmoManager::draw_triangle(const vec2& a, const vec2& b, const vec2& c, const vec3& col)
{
  m_primitives.push_back(Primitive
  {
    .type = PrimitiveType::TriangleType,
    .triangle = Triangle
    {
      .a = a,
      .b = b,
      .c = c,
      .color = col
    }
  });
}

//==============================================================================
void GizmoManager::render_line(const Line& line)
{
  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  vec3 data[2] = {vec3(line.from, -0.5f), vec3(line.to, -0.5f)};
  glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * 2, data, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 

  glUniform3f(0, line.color.x, line.color.y, line.color.z);

  glDrawArrays(GL_LINES, 0, 2);

  glDeleteBuffers(1, &vertex_buffer);
}

//==============================================================================
void GizmoManager::render_triangle(const Triangle& tri)
{
  GLuint vertex_buffer;
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  vec3 data[3] = 
  {
    vec3(tri.a, -0.5f),
    vec3(tri.b, -0.5f),
    vec3(tri.c, -0.5f),
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0); 

  glUniform3f(0, tri.color.x, tri.color.y, tri.color.z);

  glDrawArrays(GL_TRIANGLES, 0, 3);

  glDeleteBuffers(1, &vertex_buffer);
}

}

#endif

