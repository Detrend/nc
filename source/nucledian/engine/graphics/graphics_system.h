// Project Nucledian Source File
#pragma once

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/graphics/resources/model.h>
#include <engine/graphics/debug_camera.h>
#include <transform.h>

#include <types.h>
#include <config.h>
#include <vec.h>
#include <temp_math.h>

#include <vector>

struct SDL_Window;

namespace nc
{

struct VisibleSectors;
struct ModuleEvent;

class GraphicsSystem : public IEngineModule
{
public:
  static EngineModuleId get_module_id();
  bool init();
  void on_event(ModuleEvent& event) override;

  const DebugCamera& get_debug_camera() const;
  const MaterialHandle& get_solid_material() const;
  const Model& get_cube_model() const;
  DebugCamera* get_camera() const;

private:
  void update(f32 delta_seconds);
  void render();
  void terminate();

  void query_visible_sectors(VisibleSectors& out) const;

  void render_sectors(const VisibleSectors& visible)  const;
  void render_entities(const VisibleSectors& visible) const;
  void render_gun();

  void build_map_gfx() const;

  #ifdef NC_DEBUG_DRAW
  void render_map_top_down(const VisibleSectors& visible);
  void draw_debug_window();
  #endif

private:
  SDL_Window*    m_window        = nullptr;
  void*          m_gl_context    = nullptr;
  DebugCamera    m_debug_camera;
  // Material for rendering solid geometry.
  MaterialHandle m_solid_material;
  Model          m_cube_model;

  Model          m_gun_model;
  Transform      m_gun_transform;
};

}