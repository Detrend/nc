// Project Nucledian Source File
#pragma once

#include <types.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/graphics/debug_camera.h>
#include <engine/graphics/renderer.h>
#include <temp_math.h>

struct SDL_Window;

namespace nc
{

struct ModuleEvent;
struct ModelHandle;

class GraphicsSystem : public IEngineModule
{
public:
  static EngineModuleId get_module_id();
  bool init();
  void on_event(ModuleEvent& event) override;

  void update_window_and_pump_messages();

  const DebugCamera& get_debug_camera() const;
  // TODO: time to live
  /**
  * Creates a new gizmo.
  *
  * The created gizmo will remain visible until:
  * - All GizmoPtr references are destroyed, OR
  * - Time to live (ttl) expires (if specified) (ttl is future feature and is not currently implemented)
  */
  GizmoPtr create_gizmo(MeshHandle mesh_handle, const mat4& transform, const color& color);
  /**
  * Creates a new gizmo.
  *
  * The created gizmo will remain visible until:
  * - All GizmoPtr references are destroyed, OR
  * - Time to live (ttl) expires (if specified) (ttl is future feature and is not currently implemented)
  */
  GizmoPtr create_gizmo(MeshHandle mesh_handle, const vec3& position, const color& color);

private:
  void update(f32 delta_seconds);
  void render();
  void terminate();

private:
  SDL_Window* m_window       = nullptr;
  void*       m_gl_context   = nullptr;
  DebugCamera m_debug_camera;
  Renderer    m_renderer;
};

}