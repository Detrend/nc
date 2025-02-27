// Project Nucledian Source File
#pragma once

#include <types.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/graphics/debug_camera.h>

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
  void update(f32 delta_seconds);
  void render();
  void terminate();

private:
  SDL_Window* m_window         = nullptr;
  void*       m_gl_context     = nullptr;

  u32         m_vao            = 0;
  u32         m_vbo            = 0;
  // u32         m_ebo            = 0;
  u32         m_shader_program = 0;

  DebugCamera m_debug_camera;
};

}