// Project Nucledian Source File
#pragma once

#include <types.h>
#include <config.h>
#include <transform.h>
#include <math/vector.h>
#include <math/utils.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/graphics/renderer.h>
#include <engine/graphics/resources/model.h>

#include <vector>

struct SDL_Window;

namespace nc
{

struct VisibilityTree;
struct ModuleEvent;
struct Portal;

class GraphicsSystem : public IEngineModule
{
public:
  static constexpr f32 WINDOW_WIDTH  = 1024.0f;
  static constexpr f32 WINDOW_HEIGHT = 576.0f;
  // TODO: MacSectors::query_visible don't work correctly with other aspect ratios
  static constexpr f32 ASPECT_RATIO  = 800.0f / 600.0f;
  static constexpr f32 FOV = 70.0f * (1.0f / 180.0f) * PI; // 70 degrees

  static EngineModuleId  get_module_id();
  static GraphicsSystem& get();

  bool init();
  void on_event(ModuleEvent& event) override;

  const MaterialHandle& get_solid_material() const;
  const MaterialHandle& get_billboard_material() const;
  const std::vector<MeshHandle>& get_sector_meshes() const;
  const mat4& get_default_projection() const;

private:
  void update(f32 delta_seconds);
  void render();
  void terminate();

  void query_visibility(VisibilityTree& out) const;
  void create_sector_meshes();

  #ifdef NC_DEBUG_DRAW
  void render_map_top_down(const VisibilityTree& visible);
  void draw_debug_window();
  #endif

private:
  SDL_Window* m_window     = nullptr;
  void*       m_gl_context = nullptr;

  RendererPtr             m_renderer           = nullptr;
  std::vector<MeshHandle> m_sector_meshes;
  mat4                    m_default_projection;

  MaterialHandle m_solid_material;
  MaterialHandle m_billboard_material;
};

}