#include <engine/editor/editor_module.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>

namespace nc
{
  //=========================================================
  EngineModuleId EditorSystem::get_module_id()
  {
    return EngineModule::editor_system;
  }
  //=========================================================

  void EditorSystem::on_event(ModuleEvent& event)
  {
    switch (event.type)
    {
    case ModuleEventType::editor_update_2d:
    {
      break;
    }
    case ModuleEventType::editor_update_3d:
    {
      break;
    }
    case ModuleEventType::editor_render_2d:
    {
      break;
    }
    case ModuleEventType::editor_render_3d:
    {
      break;
    }
    default:
      break;
    }
  }
}