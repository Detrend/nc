#include <engine/editor/editor_module.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>

#include <vector>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_opengl3.h>

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
    case ModuleEventType::terminate:
    {
      break;
    }
    default:
      break;
    }
  }

  EditorSystem::~EditorSystem()
  {
    
  }

  bool EditorSystem::init()
  {
    return true;
  }

  //===========================================================

  /**
  void EditorSystem::render_grid()
  {
    grid.render_grid(xOffset, yOffset);
  }
  */

  //===========================================================

  void Grid::init()
  {
    points.resize((100 + 1) * 2 * 2);

    size_t i = 0;
    for (f64 x = -50; x <= 50 + GRID_SIZE / 2; x += GRID_SIZE)
    {
      points[i] = vertex_3d(x, -50, 0);
      points[i + 1] = vertex_3d(x, 50, 0);
      points[i + 2] = vertex_3d(-50, x, 0);
      points[i + 3] = vertex_3d(50, x, 0);

      i += 4;
    }

    // bind to VBO
  }

  Grid::~Grid()
  {
  }

  //===========================================================
}