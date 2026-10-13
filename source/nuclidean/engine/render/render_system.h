// Project Nuclidean Source File
#pragma once

#include <config.h>
#include <types.h>
#include <math/vector.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/core/module_event.h>
#include <engine/render/sdl_types.h>
#include <engine/render/texture_atlas.h>
#include <engine/render/renderers/base_renderer.h>

struct SDL_Window;

namespace nc
{

class RenderSystem : public IEngineModule
{
public:
  static EngineModuleId get_module_id();
  static RenderSystem& get();

  bool init();
  void terminate();
  void on_event(ModuleEvent& event) override;
  // Called when the sector floor or ceiling moved this frame.
  void on_sector_changed(SectorID sector_id) { m_renderer->on_sector_changed(sector_id); }
  // Called when the wall segment switched between its default and triggered texture.
  void on_segment_changed(SegmentID segment_id) { m_renderer->on_segment_changed(segment_id); }

  template<typename TRenderer>
  bool switch_renderer();

  bool recreate_window(SDL_RenderBackend backend);
  SDL_Window* get_window() const { return m_window; }
  // Window size in pixels, cached on each render.
  vec2 get_window_size() const { return vec2(m_window_resolution); }
  // Textures used during the whole game.
  const render::TextureAtlas& get_game_atlas() const { return m_game_atlas; }

private:
  bool set_renderer(render::renderers::BaseRendererPtr renderer);
  void render();

  render::renderers::BaseRendererPtr m_renderer;
  SDL_Window* m_window = nullptr;
  // Window size in pixels, cached on each render.
  uvec2 m_window_resolution = uvec2(0, 0);
  // Resolution of the image the game renders into.
  uvec2 m_virtual_resolution = uvec2(0, 0);
  // Textures used during the whole game.
  render::TextureAtlas m_game_atlas;
};

}

#include <engine/render/render_system.inl>
