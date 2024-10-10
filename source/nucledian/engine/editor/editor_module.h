#pragma once

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>

namespace nc
{
  enum editorView
  {
    view_2d,
    view_3d
  };

  struct ModuleEvent;

  class EditorSystem : public IEngineModule
  {
  public:
    static EngineModuleId get_module_id();
    void on_event(ModuleEvent& event) override;

  private:
    editorView view;
  };

  
}