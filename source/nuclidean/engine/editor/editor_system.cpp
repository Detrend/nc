// Project Nuclidean Source File
#include <common.h>
#include <config.h>

#include <engine/editor/editor_system.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/engine.h>
#include <engine/core/module_event.h>

#include <engine/editor/editor.h>

namespace nc
{

//==================================================================================================
/*static*/ EngineModuleId EditorSystem::get_module_id()
{
  return EngineModule::editor_system;
}

//==================================================================================================
/*static*/ EditorSystem& EditorSystem::get()
{
  return get_engine().get_module<EditorSystem>();
}

//==================================================================================================
bool EditorSystem::init()
{
  return true;
}

//==================================================================================================
void EditorSystem::on_event(ModuleEvent& event)
{
  switch (event.type)
  {
    case ModuleEventType::editor_start_request:
    {
      nc_assert(!m_editor);
      m_editor = std::make_unique<Editor>();
      m_editor->init();
    }
    break;

    case ModuleEventType::editor_end_request:
    {
      nc_assert(m_editor);
      m_editor->terminate();
      m_editor.reset();
    }
    break;

    case ModuleEventType::editor_update:
    {
      nc_assert(m_editor);
      m_editor->update(event.update.dt);
    }
    break;
  }
}

}
