// Project Nucledian Source File
#pragma once

#include <types.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>

struct SDL_Window;

namespace nc
{

struct ModuleEvent;

class GraphicsSystem : public IEngineModule
{
public:
  static EngineModuleId get_module_id();
  bool init();
  void on_event(ModuleEvent& event) override;

  void update_window_and_pump_messages();

private:
  void terminate();
  void render();

private:
  SDL_Window* m_window     = nullptr;
  void*       m_gl_context = nullptr;
  u32         m_vao        = 0;
};

}