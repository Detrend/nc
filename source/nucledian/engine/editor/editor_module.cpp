#include <engine/editor/editor_module.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>

#include <vector>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_opengl3.h>

// #define GL_GLEXT_PROTOTYPES
#include <gl/GL.h>
#include <gl/GLU.h>

#include <SDL.h>
#include <SDL_opengl.h>
// #include <SDL_opengl_glext.h>


namespace nc
{
  EditorSystem::EditorSystem()
  {
  }
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
    case ModuleEventType::post_init:
    {
      break;
    }
    case ModuleEventType::editor_update:
    {
      
      break;
    }
    case ModuleEventType::editor_render:
    {
      draw_ui();
      break;
    }
    case ModuleEventType::cleanup:
    {
      break;
    }
    case ModuleEventType::terminate:
    {
      terminate_imgui();
      break;
    }
    default:
      break;
    }
  }

  EditorSystem::~EditorSystem()
  {
    
  }

  bool EditorSystem::init(SDL_Window* window, void* gl_context)
  {
    glOrtho(64, 64, 48, 48, -10, 10);

    grid.init();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    return true;
  }

  void EditorSystem::draw_ui()
  {
  }

  void EditorSystem::terminate_imgui()
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
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
    points.resize((400 + 1) * 2 * 2);

    size_t i = 0;
    for (f32 x = -50; x <= 50 + GRID_SIZE / 2; x += GRID_SIZE)
    {
      points[i] = vertex_3d(x, -50, 0);
      points[i + 1] = vertex_3d(x, 50, 0);
      points[i + 2] = vertex_3d(-50, x, 0);
      points[i + 3] = vertex_3d(50, x, 0);

      i += 4;
    }

    // bind to VBO
    //glGenBuffers(1, &vertex_buffer);
    //glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * points.size(), &points.front(), GL_STATIC_DRAW);

  }

  Grid::~Grid()
  {
  }

  


  //===========================================================
}