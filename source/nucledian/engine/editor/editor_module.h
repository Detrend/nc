#pragma once

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>

#include <engine/editor/map_info.h>

namespace nc
{
  const f64 GRID_SIZE = 0.25;

  enum editorView
  {
    view_2d,
    view_3d
  };

  struct ModuleEvent;
  
  class Grid {
  public:
    void init();
    void render_grid(f64 xOffset, f64 yOffset);

  private:  
    std::vector<vertex_3d> points;
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

    // mouse checkers for manipulating with drawing

    vertex_2d getMousePos();
    vertex_2d getPrevMousePos();
    vertex_2d getMouseShift();
    vertex_2d applyMouseShift();

    //

    editorView view;
    Grid grid;

    //
    
    vertex_2d prevMousePos;
    vertex_2d curMousePos;
    
    double xOffset;
    double yOffset;
  };

  
  
}