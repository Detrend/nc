#pragma once

#include <glad/glad.h>#

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>

#include <engine/editor/map_info.h>

#include <imgui/imgui.h>
//#include <imgui/imgui_impl_sdl2.h>
//#include <imgui/imgui_impl_opengl3.h>

#include <SDL.h>

#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace nc
{
  const f64 GRID_SIZE = 0.25;

  enum editorView
  {
    view_2d,
    view_3d
  };

  enum editorMode2D
  {
    move,
    vertex,
    line,
    sector
  };

  struct ModuleEvent;
  
  class Grid {
  public:
    
    void init();
    void render_grid(vertex_2d windowSize, vertex_2d offset, float zoom);
    ~Grid();

  private:  
    std::vector<vertex_3d> points;
    // definition of the lines themeself

    GLuint vertexBuffer;
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint shaderProgram;

    GLuint vertexArrayBuffer;
    glm::mat4 viewMatrix;
  };

  class EditorSystem : public IEngineModule
  {
  public:
    EditorSystem() : io(*new ImGuiIO()) {}
    static EngineModuleId get_module_id();
    void on_event(ModuleEvent& event) override;
    ~EditorSystem();

    bool init(SDL_Window* window, void* gl_context);

  private:
    void initImGui(SDL_Window* window, void* gl_context);
    void draw_ui(vertex_2d windowSize);
    void CreateBottomBar(vertex_2d& windowSize);
    void CreateMenuBar();
    void terminate_imgui();

    void getMouseInput();
    void getLeftMouseButton(Uint32 mouseState);
    vertex_2d getMousePos(int x, int y);
    vertex_2d getSnapToGridPos(float x, float y);

    editorView view;
    Grid grid;

    bool prevLeftMouse[5] = {false, false, false, false, false};
    bool curLeftMouse[5] = {false, false, false, false, false};

    vertex_2d windowSize;
    
    vertex_2d prevMousePos;
    vertex_2d curMousePos;
    vertex_2d prevGridMousePos;
    vertex_2d curGridMousePos;

    vertex_2d gridOffset;

    SDL_Window* window;
    ImGuiIO& io;

    float zoom;
    editorMode2D editMode2D;
  };

  
  
}