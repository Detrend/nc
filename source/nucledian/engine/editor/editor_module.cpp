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
    case ModuleEventType::editor_update:
    {
      break;
    }
    case ModuleEventType::editor_render:
    {
      break;
    }
    default:
      break;
    }
  }

  //===========================================================

  void EditorSystem::init()
  {
    grid = Grid();
    grid.init();
  }

  void EditorSystem::render_grid()
  {
    grid.render_grid(xOffset, yOffset);
  }

  //=======================================================
  // Mouse functions for determining the shift

  vertex_2d EditorSystem::get_current_mouse_pos()
  {
    // NEED TO GET MOUSE POSITION FROM WINDOW
    return vertex_2d(0, 0);
  }

  vertex_2d EditorSystem::get_prev_mmouse_pos()
  {
    return prevMousePos;
  }

  vertex_2d EditorSystem::get_mouse_shift()
  {
    return curMousePos - prevMousePos;
  }
  
  //===========================================================
  
  Grid::Grid()
  {
  }

  //===========================================================

  void Grid::init()
  {
    points = std::vector<vertex_3d>();
    // This might be better done with instancing
    points.resize((100 * 4 + 1) * 2);

    size_t i = 0;
    for (f64 x = -50; x <= 50 + 0.1; x += GRID_SIZE) {
      points[i] = vertex_3d(x, -50, 0);
      points[i + 1] = vertex_3d(x, 50, 0);
      points[i + 2] = vertex_3d(-50, x, 0);
      points[i + 3] = vertex_3d(50, x, 0);

      i+=4;
    }
  }

  //=========================================================

  void Grid::render_grid(double xOffset, double yOffset)
  {
    f64 xSteps = floor(xOffset / GRID_SIZE);
    f64 ySteps = floor(yOffset / GRID_SIZE);

    f64 xShift = xSteps * GRID_SIZE;
    f64 yShift = ySteps * GRID_SIZE;
    
    // Render -> send to openGL with this shift as uniform
  }

  //=======================================================
}