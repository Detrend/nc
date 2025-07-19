#pragma once

#include <config.h>

#ifdef NC_DEBUG_DRAW

#include <math/vector.h>
#include <engine/graphics/resources/mesh.h>
#include <engine/graphics/resources/material.h>

#include <memory>
#include <unordered_map>

namespace nc
{

// Forward declare so we do not need to include graphics system
struct CameraData;

/**
  * Temporary visual marker for debugging. Gizmos are light-weight render objects that can be created and destroyed at
  * runtime.
  */
class Gizmo
{
public:
  friend class GizmoManager;

  /**
   * Creates a new cube gizmo. Gizmo will remain visible until all GizmoPtr references are destroyed.
   */
  static std::shared_ptr<Gizmo> create_cube(const vec3& position, f32 size = 0.1f, const color4& color = colors::RED);
  /**
   * Creates a new cube gizmo. Gizmo will remain visible until time to live (TTL) reaches zero.
   */
  static void create_cube(f32 ttl, const vec3& position, f32 size = 0.1f, const color4& color = colors::RED);

  /**
   * Creates a new ray gizmo. Gizmo will remain visible until all GizmoPtr references are destroyed.
   */
  static std::shared_ptr<Gizmo> create_ray(
    const vec3& start,
    const vec3& direction,
    const color4& color = colors::RED
  );
  /**
   * Creates a new ray gizmo. Gizmo will remain visible until time to live (TTL) reaches zero.
   */
  static void create_ray(f32 ttl, const vec3& start, const vec3& direction, const color4& color = colors::RED);

  /**
   * Creates a new line gizmo. Gizmo will remain visible until all GizmoPtr references are destroyed.
   */
  static std::shared_ptr<Gizmo> create_line(const vec3& start, const vec3& end, const color4& color = colors::RED);
  /**
  * Creates a new line gizmo. Gizmo will remain visible until time to live (TTL) reaches zero.
  */
  static void create_line(f32 ttl, const vec3& start, const vec3& end, const color4& color = colors::RED);

private:
  Gizmo(const MeshHandle& mesh, const mat4& transform, const color4& color, f32 ttl);

  static std::shared_ptr<Gizmo> create_cube_impl(f32 ttl, const vec3& position, f32 size, const color4& color);
  static std::shared_ptr<Gizmo> create_ray_impl(f32 ttl, const vec3& start, const vec3& direction, const color4& color);
  static std::shared_ptr<Gizmo> create_line_impl(f32 ttl, const vec3& start, const vec3& end, const color4& color);

  static std::pair<vec3, f32> compute_rotation_angle_axis(const vec3& direction);

  // time to live
  f32 m_ttl = 0.0f;

  const MeshHandle  m_mesh;
  const color4       m_color     = colors::WHITE;
  const mat4        m_transform = mat4(1.0f);
};
/**
  * Smart pointer managing the lifetime of a Gizmo instance.
  * The referenced gizmo will be automatically destroyed when:
  * - All owning GizmoPtr instances are destroyed, OR
  * - Specified time-to-live duration expires (when used)
  */
using GizmoPtr = std::shared_ptr<Gizmo>;

class GizmoManager
{
public:
  friend class Gizmo;

  static GizmoManager& instance();

  // Update time to live (TTL) of active gizmos.
  void update_ttls(f32 delta_seconds);
  void draw_gizmos(const CameraData& camera) const;

private:
  GizmoManager() {}

  GizmoPtr add_gizmo(Gizmo gizmo, bool use_ttl);

  using GizmoMap = std::unordered_map<u32, Gizmo>;

  inline static std::unique_ptr<GizmoManager> m_instance = nullptr;
  inline static u32 m_next_gizmo_id = 0;

  // Contain all active gizmos with GizmoPtr lifetime management.
  GizmoMap m_gizmos;
  // Contain all active gizmos with TTL lifetime management.
  GizmoMap m_ttl_gizmos;
};

}

#endif

