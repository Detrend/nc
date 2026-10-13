// Project Nuclidean Source File
#include <engine/render/render_system.h>

#include <common.h>
#include <config.h>
#include <types.h>
#include <logging.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/render/sdl_utils.h>
#include <engine/render/renderers/opengl_ray_tracer.h>

#if NC_IMGUI
#include <imgui/imgui.h>
#include <imgui/implot.h>
#endif

#include <SDL.h>
#include <SDL_video.h>

namespace nc
{

using namespace render;

//==============================================================================
EngineModuleId RenderSystem::get_module_id()
{
  return EngineModule::render_system;
}

//==============================================================================
RenderSystem& RenderSystem::get()
{
  return get_engine().get_module<RenderSystem>();
}

//==============================================================================
bool RenderSystem::init()
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    return sdl_crit("SDL_Init");

#if NC_IMGUI
  ImGui::CreateContext();
  ImPlot::CreateContext();
#endif

  if (!switch_renderer<renderers::OpenGLRayTracer>())
    return false;

#if NC_IMGUI
  ImGui::NewFrame();
#endif

  return true;
}

//==============================================================================
void RenderSystem::terminate()
{
  if (m_renderer)
  {
    m_renderer->terminate();
    m_renderer = nullptr;
  }

#if NC_IMGUI
  if (ImPlot::GetCurrentContext())
    ImPlot::DestroyContext();

  if (ImGui::GetCurrentContext())
    ImGui::DestroyContext();
#endif

  if (m_window)
  {
    SDL_DestroyWindow(m_window);
    m_window = nullptr;
  }

  SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
}

//==============================================================================
void RenderSystem::on_event(ModuleEvent& event)
{
  switch (event.type)
  {
    case ModuleEventType::render:
      this->render();
      break;

    case ModuleEventType::terminate:
      this->terminate();
      break;
  }
}

//==============================================================================
void RenderSystem::render()
{
#if NC_HOT_RELOAD
  m_renderer->reload_shaders();
#endif

  s32 width = 0;
  s32 height = 0;
  SDL_GetWindowSizeInPixels(m_window, &width, &height);

  const uvec2 window_size(width, height);
  if (window_size != m_window_size)
  {
    m_window_size = window_size;
    m_renderer->on_window_resized(m_window_size.x, m_window_size.y);
  }

  m_renderer->render();

#if NC_IMGUI
  ImGui::NewFrame();
#endif
}

//==============================================================================
bool RenderSystem::recreate_window(SDL_RenderBackend backend)
{
  s32 x = SDL_WINDOWPOS_UNDEFINED;
  s32 y = SDL_WINDOWPOS_UNDEFINED;
  s32 width = 1280;
  s32 height = 720;

  if (m_window)
  {
    SDL_GetWindowPosition(m_window, &x, &y);
    SDL_GetWindowSize(m_window, &width, &height);
    SDL_DestroyWindow(m_window);
    m_window = nullptr;
  }

  m_window = SDL_CreateWindow(
    "Nuclidean",
    x,
    y,
    width,
    height,
    backend | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
  );
  if (!m_window)
    return sdl_warn("SDL_CreateWindow");

  SDL_SetWindowResizable(m_window, get_engine().is_editor_mode() ? SDL_TRUE : SDL_FALSE);
  return true;
}

//==============================================================================
bool RenderSystem::set_renderer(renderers::BaseRendererPtr renderer)
{
  renderers::BaseRendererPtr previous = std::move(m_renderer);
  if (previous)
    previous->terminate();

  if (renderer->init())
  {
    m_renderer = std::move(renderer);
    return true;
  }
  renderer->terminate();

  if (previous)
  {
    nc_warn("[render][render system] failed to init renderer, switching back to the previous one");
    if (previous->init())
    {
      m_renderer = std::move(previous);
      return true;
    }
    previous->terminate();
  }

  nc_crit("[render][render system] failed to init renderer, no renderer is running");
  return false;
}

}
