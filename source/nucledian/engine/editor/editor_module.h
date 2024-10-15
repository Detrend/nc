#pragma once

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/editor/map_info.h>

#include <vector>

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
    Grid();
    void init();
    void render_grid(double xOffset, double yOffset);
    
  private:

    // definition of the lines themeself
    // each line is two points
    std::vector<vertex_3d> points;
  };

  class EditorSystem : public IEngineModule
  {
  public:
    static EngineModuleId get_module_id();
    void on_event(ModuleEvent& event) override;
    void init();

    void render_grid();
    void render_map_2d();

  private:
    vertex_2d get_current_mouse_pos();
    vertex_2d get_prev_mmouse_pos();
    vertex_2d get_mouse_shift();

    vertex_2d prevMousePos;
    vertex_2d curMousePos;
    double xOffset;
    double yOffset;
    editorView view;
    Grid grid;
  };

  
  
}