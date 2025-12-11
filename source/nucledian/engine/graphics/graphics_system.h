// Project Nucledian Source File
#pragma once

#include <types.h>
#include <config.h>
#include <math/vector.h>
#include <math/utils.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/graphics/resources/model.h>
#include <engine/map/map_types.h>            // SectorID

#include <game/game_types.h>

#include <vector>
#include <string>

struct SDL_Window;

namespace nc
{

struct VisibilityTree;
struct ModuleEvent;
struct Portal;

struct CameraData
{
	const vec3& position;
	const mat4& view;
	const mat4& projection;
	const VisibilityTree& vis_tree;
};

struct RenderGunProperties
{
  vec2        sway   = VEC2_ZERO;
  std::string sprite = "";
};

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

  GraphicsSystem();
  ~GraphicsSystem();

  GraphicsSystem(const GraphicsSystem&)            = delete;
  GraphicsSystem& operator=(const GraphicsSystem&) = delete;

  bool init();
  void on_event(ModuleEvent& event) override;

  // This returns the mesh handle. If the sector is dirty (was changed)
  // then it is recomputed.
  const MeshHandle& get_and_update_sector_mesh(SectorID sector);
  void              mark_sector_dirty(SectorID sector);

  const ShaderProgramHandle& get_solid_material() const;

  vec2 get_window_size();

  SDL_Window* get_window();

private:
  void update(f32 delta_seconds);
  void render();
  void terminate();

  void query_visibility(VisibilityTree& out) const;
  void create_sector_meshes();

#ifdef NC_DEBUG_DRAW
  void render_map_top_down(const VisibilityTree& visible);
  void draw_debug_window();
  void handle_light_debug();
  void handle_sector_height_debug();
#endif

private:
  // Because we do not want to include the whole renderer with this header
  using RendererPtr = std::unique_ptr<class Renderer>;

  SDL_Window* m_window     = nullptr;
  void*       m_gl_context = nullptr;

  RendererPtr             m_renderer = nullptr;
  std::vector<MeshHandle> m_sector_meshes;
  std::vector<bool>       m_dirty_sectors;

#ifdef NC_DEBUG_DRAW
  using DebugRendererPtr = std::unique_ptr<class TopDownDebugRenderer>;
  DebugRendererPtr m_debug_renderer = nullptr;
#endif

  u32 m_window_width  = static_cast<u32>(WINDOW_WIDTH);
  u32 m_window_height = static_cast<u32>(WINDOW_HEIGHT);
};

}