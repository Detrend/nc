#pragma once

#include <config.h>

#ifdef NC_DEBUG_DRAW

#include <temp_math.h>
#include <engine/graphics/resources/mesh.h>
#include <engine/graphics/resources/material.h>

#include <memory>
#include <unordered_map>

namespace nc
{

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
  static std::shared_ptr<Gizmo> create_cube(const vec3& pos, f32 size = 0.1f, const color& color = colors::RED);
  /**
   * Creates a new cube gizmo. Gizmo will remain visible until time to live (ttl) reaches zero.
   */
  static void create_cube(f32 ttl, const vec3& pos, f32 size = 0.1f, const color& color = colors::RED);

  // TODO: create_line

private:
  Gizmo(const Mesh& mesh, const vec3& pos, f32 size, const color& color, f32 ttl);

  // time to live
  f32 m_ttl = 0.0f;

  const Mesh  m_mesh;
  const color m_color     = colors::WHITE;
  const mat4  m_transform = mat4(1.0f);
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

  // Update time to live (ttl) of active gizmos.
  void update_ttls(f32 delta_seconds);
  void draw_gizmos() const;

private:
  using GizmoMap = std::unordered_map<u32, Gizmo>;

  inline static std::unique_ptr<GizmoManager> m_instance = nullptr;
  inline static u32 m_next_gizmo_id = 0;

  // Contain all active gizmos with GizmoPtr lifetime managment.
  GizmoMap m_gizmos;
  // Contain all active gizmos with ttl lifetime managment.
  GizmoMap m_ttl_gizmos;

  GizmoManager() {}
};

}

#endif

