// Project Nucledian Source File
#pragma once

#include <types.h>
#include <temp_math.h>
#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/graphics/gizmo.h>
#include <engine/graphics/resources/mesh.h>
#include <engine/graphics/resources/model.h>
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

  const DebugCamera& get_debug_camera() const;
  MeshManager& get_mesh_manager();
  ModelManager& get_model_manager();
  GizmoManager& get_gizmo_manager();

  const Material& get_solid_material() const;
  ModelHandle get_cube_model_handle() const;

private:
  void update(f32 delta_seconds);
  void render();
  void terminate();

  void render_entities() const;

  SDL_Window*  m_window        = nullptr;
  void*        m_gl_context    = nullptr;
  DebugCamera  m_debug_camera;
  MeshManager  m_mesh_manager;
  ModelManager m_model_manager;
  GizmoManager m_gizmo_manager;

  // Material for rendering solid geometry.
  Material     m_solid_material;
  ModelHandle  m_cube_model_handle = ModelHandle::invalid();
};

}