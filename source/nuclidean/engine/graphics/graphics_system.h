// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <config.h>
#include <math/vector.h>
#include <math/utils.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/map/map_types.h>            // SectorID

#include <game/game_types.h>
#include <cvars.h>

#include <optional>
#include <vector>
#include <string>

struct SDL_Window;

namespace nc
{

struct VisibilityTree;
struct ModuleEvent;
struct Portal;
class  ShaderProgramHandle;
class  GameThreadRenderCommandCollector;
class  RenderProxyRegistry;

struct CameraData
{
  const vec3& position;
  const mat4& view;
  const mat4& projection;
  const VisibilityTree& vis_tree;
};

enum class RenderBackend {
  None,
  OpenGL,
  OpenGLES
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
  ~GraphicsSystem() override;

  GraphicsSystem(const GraphicsSystem&)            = delete;
  GraphicsSystem& operator=(const GraphicsSystem&) = delete;

  bool init(bool force_gles);
  void on_event(ModuleEvent& event) override;

  const ShaderProgramHandle& get_solid_material() const;

  vec2 get_window_size();

  SDL_Window* get_window();
  void set_shadows_game_thread(bool shadows);

  RenderBackend get_backend() const { return m_backend; }

  GameThreadRenderCommandCollector& get_command_collector();
  RenderProxyRegistry&              get_render_proxies();
  const RenderProxyRegistry&        get_render_proxies() const;

private:
  void update(f32 delta_seconds);
  void render();
  void terminate();

  void query_visibility(VisibilityTree& out) const;
  struct GlLoadingInstructions;
  std::optional<std::string> try_create_opengl_context(const GlLoadingInstructions& instructions);

#if NC_DEBUG_DRAW
  void render_map_top_down(const VisibilityTree& visible);
  void draw_debug_window();
#endif

private:
  // Because we do not want to include the whole renderer with this header
  using RendererPtr         = std::unique_ptr<class Renderer>;
  using UiRendererPtr       = std::unique_ptr<class UiRenderer>;
  using CommandCollectorPtr = std::unique_ptr<GameThreadRenderCommandCollector>;
  using RenderProxiesPtr    = std::unique_ptr<RenderProxyRegistry>;

  SDL_Window* m_window     = nullptr;
  void*       m_gl_context = nullptr;

  RendererPtr         m_renderer;
  UiRendererPtr       m_ui_renderer;
  RenderBackend       m_backend;
  CommandCollectorPtr m_command_collector;
  RenderProxiesPtr    m_render_proxies;

#if NC_DEBUG_DRAW
  using DebugRendererPtr = std::unique_ptr<class TopDownDebugRenderer>;
  DebugRendererPtr m_debug_renderer;
#endif

  u32 m_window_width  = cast<u32>(WINDOW_WIDTH);
  u32 m_window_height = cast<u32>(WINDOW_HEIGHT);
  f32 m_last_resolution_scale = CVars::resolution_scale; // Resolution scale used on last frame, stored just so we can detect the cvar's change
};

}
