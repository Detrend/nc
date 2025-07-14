#include <engine/graphics/gizmo.h>

#ifdef NC_DEBUG_DRAW

#include <math/lingebra.h>

#include <engine/core/engine.h>
#include <engine/graphics/camera.h>
#include <engine/graphics/shaders/shaders.h>
#include <engine/graphics/graphics_system.h>

#include <ranges>
#include <array>

namespace nc
{

//==============================================================================
GizmoPtr Gizmo::create_cube(const vec3& position, f32 size, const color4& color)
{
  return create_cube_impl(0.0f, position, size, color);
}

//==============================================================================
void Gizmo::create_cube(f32 ttl, const vec3& position, f32 size, const color4& color)
{
  create_cube_impl(ttl, position, size, color);
}

//==============================================================================
std::shared_ptr<Gizmo> Gizmo::create_ray(const vec3& start, const vec3& direction, const color4& color)
{
  return create_ray_impl(0.0f, start, direction, color);
}

//==============================================================================
void Gizmo::create_ray(f32 ttl, const vec3& start, const vec3& direction, const color4& color)
{
  create_ray_impl(ttl, start, direction, color);
}

//==============================================================================
std::shared_ptr<Gizmo> Gizmo::create_line(const vec3& start, const vec3& end, const color4& color)
{
  return create_line_impl(0.0f, start, end, color);
}

//==============================================================================
void Gizmo::create_line(f32 ttl, const vec3& start, const vec3& end, const color4& color)
{
  create_line_impl(ttl, start, end, color);
}

//==============================================================================
Gizmo::Gizmo(const MeshHandle& mesh, const mat4& transform, const color4& color, f32 ttl)
  : m_ttl(ttl), m_mesh(mesh), m_color(color), m_transform(transform) {}

//==============================================================================
std::shared_ptr<Gizmo> Gizmo::create_cube_impl(f32 ttl, const vec3& position, f32 size, const color4& color)
{
  const mat4 transform = translate(mat4(1.0f), position) * scale(mat4(1.0f), vec3(size));

  GizmoManager& gizmo_manager = GizmoManager::instance();
  return gizmo_manager.add_gizmo(Gizmo(MeshManager::instance().get_cube(), transform, color, ttl), ttl > 0.0f);
}

//==============================================================================
std::shared_ptr<Gizmo> Gizmo::create_ray_impl(f32 ttl, const vec3& start, const vec3& direction, const color4& color)
{
  const auto [axis, angle] = compute_rotation_angle_axis(direction);

  const mat4 transform = translate(mat4(1.0f), start)
    * scale(mat4(1.0f), vec3(1e6f))
    * rotate(mat4(1.0f), angle, axis);
  
  GizmoManager& gizmo_manager = GizmoManager::instance();
  return gizmo_manager.add_gizmo(Gizmo(MeshManager::instance().get_line(), transform, color, ttl), ttl > 0.0f);
}

//==============================================================================
std::shared_ptr<Gizmo> Gizmo::create_line_impl(f32 ttl, const vec3& start, const vec3& end, const color4& color)
{
  const vec3 direction = end - start;
  const auto [axis, angle] = compute_rotation_angle_axis(direction);

  const mat4 transform = translate(mat4(1.0f), start)
    * scale(mat4(1.0f), vec3(length(direction)))
    * rotate(mat4(1.0f), angle, axis);

  [[maybe_unused]] const vec3 recomp_start = transform * vec4{0, 0, 0, 1};
  [[maybe_unused]] const vec3 recomp_end   = transform * vec4{1, 0, 0, 1};

  GizmoManager& gizmo_manager = GizmoManager::instance();
  return gizmo_manager.add_gizmo(Gizmo(MeshManager::instance().get_line(), transform, color, ttl), ttl > 0.0f);
}

//==============================================================================
std::pair<vec3, f32> Gizmo::compute_rotation_angle_axis(const vec3& direction)
{
  const vec3 normalized_direction = normalize_or_zero(direction);

  nc_assert(normalized_direction != VEC3_ZERO);

  constexpr vec3 base_direction = VEC3_X;
  const f32 angle = acos(dot(base_direction, normalized_direction));

  vec3 axis;
  if (base_direction == normalized_direction || -base_direction == normalized_direction)
    // angle is zero, so rotation axis don't matter
    axis = VEC3_Z;
  else
    axis = normalize(cross(base_direction, normalized_direction));

  return {axis, angle};
}

//==============================================================================
GizmoManager& GizmoManager::instance()
{
  if (m_instance == nullptr)
  {
    m_instance = std::unique_ptr<GizmoManager>(new GizmoManager());
  }

  return *m_instance;
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
  auto& graphics_system = get_engine().get_module<GraphicsSystem>();
  const Camera* camera = Camera::get();
  if (camera == nullptr)
    return;

  const MaterialHandle& solid_material = graphics_system.get_solid_material();
  solid_material.use();
  solid_material.set_uniform(shaders::solid::VIEW, camera->get_view());
  solid_material.set_uniform(shaders::solid::VIEW_POSITION, camera->get_position());

  auto combined_view = std::ranges::views::join
  (
    std::array{ std::views::all(m_gizmos), std::views::all(m_ttl_gizmos) }
  );

  for (const auto& [_, gizmo] : combined_view)
  {
    solid_material.set_uniform(shaders::solid::UNLIT, gizmo.m_mesh.get_draw_mode() == GL_LINES);
    solid_material.set_uniform(shaders::solid::COLOR, gizmo.m_color);
    solid_material.set_uniform(shaders::solid::TRANSFORM, gizmo.m_transform);

    glBindVertexArray(gizmo.m_mesh.get_vao());
    glDrawArrays(gizmo.m_mesh.get_draw_mode(), 0, gizmo.m_mesh.get_vertex_count());
  }
}

//==============================================================================
GizmoPtr GizmoManager::add_gizmo(Gizmo gizmo, bool use_ttl)
{
  const u32 id = m_next_gizmo_id++;

  GizmoMap& gizmo_map = use_ttl ? m_ttl_gizmos : m_gizmos;

  auto [it, _] = gizmo_map.emplace(id, gizmo);

  if (use_ttl)
    return nullptr;

  auto deleter = [this, id](const Gizmo*)
  {
    auto it = m_gizmos.find(id);
    if (it == m_gizmos.end())
    {
      return;
    }

    m_gizmos.erase(it);
  };

  return GizmoPtr(&(it->second), deleter);
}

}

#endif
