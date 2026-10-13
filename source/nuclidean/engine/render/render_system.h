// Project Nuclidean Source File
#pragma once

#include <config.h>
#include <types.h>
#include <math/vector.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/core/module_event.h>
#include <engine/render/sdl_types.h>
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

  template<typename TRenderer>
  bool switch_renderer();

  bool recreate_window(SDL_RenderBackend backend);
  SDL_Window* get_window() const { return m_window; }
  // Window size in pixels, cached on each render.
  vec2 get_window_size() const { return vec2(m_window_size); }

private:
  bool set_renderer(render::renderers::BaseRendererPtr renderer);
  void render();

  render::renderers::BaseRendererPtr m_renderer;
  SDL_Window* m_window = nullptr;
  // Window size in pixels, cached on each render.
  uvec2 m_window_size = uvec2(0, 0);
};

}

#include <engine/render/render_system.inl>
