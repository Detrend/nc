// Project Nucledian Source File
#pragma once

#include <types.h>
#include <config.h>
#include <transform.h>
#include <math/vector.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/graphics/resources/model.h>
#include <engine/graphics/debug_camera.h>

#include <vector>

struct SDL_Window;

namespace nc
{

struct VisibilityTree;
struct ModuleEvent;
struct PortalRenderData;

class GraphicsSystem : public IEngineModule
{
public:
  static EngineModuleId get_module_id();

  GraphicsSystem();

  bool init();
  void on_event(ModuleEvent& event) override;

  const MaterialHandle& get_solid_material() const;
  const Model& get_cube_model() const;
  const DebugCamera* get_camera() const;
  const mat4 get_default_projection() const;

private:
  struct CameraData;

  void update(f32 delta_seconds);
  void render();
  void terminate();

  void query_visibility(VisibilityTree& out) const;

  void render_sectors(const CameraData& camera_data)  const;
  void render_entities(const CameraData& camera_data) const;
  void render_portals(const CameraData& camera_data) const;
  void render_gun(const CameraData& camera_data) const;

  void render_portal_to_stencil(const CameraData& camera_data, const PortalRenderData& portal) const;
  void render_portal_to_color(const CameraData& camera_data, const PortalRenderData& portal) const;

  mat4 clip_projection(const CameraData& camera_data, const PortalRenderData& portal) const;

  void build_map_gfx() const;

  #ifdef NC_DEBUG_DRAW
  void render_map_top_down(const VisibilityTree& visible);
  void draw_debug_window();
  #endif

private:
  const mat4     m_default_projection;

  SDL_Window*    m_window        = nullptr;
  void*          m_gl_context    = nullptr;

  // Material for rendering solid geometry.
  MaterialHandle m_solid_material;
  Model          m_cube_model;

  Model          m_gun_model;
  Transform      m_gun_transform;

  struct CameraData
  {
    const vec3& position;
    const mat4& view;
    const mat4& projection;
    const VisibilityTree& vis_tree;
  };
};

}