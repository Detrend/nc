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
  
  class Grid {
    double xOffset;
    double yOffset;

    // definition of the lines themeself
  };

  class EditorSystem : public IEngineModule
  {
  public:
    static EngineModuleId get_module_id();
    void on_event(ModuleEvent& event) override;

    void render_grid();
    void render_map_2d();

  private:
    editorView view;
    Grid grid;
  };

  
  
}