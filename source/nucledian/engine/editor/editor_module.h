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
    EditorSystem() : io(* new ImGuiIO()) {}
    static EngineModuleId get_module_id();  
    void on_event(ModuleEvent& event) override;
    ~EditorSystem();

    bool init(SDL_Window* window, void* gl_context);
    
    void render_grid();
    void render_map_2d();

    void destroy();

  private:
    // imgui skeleton
    void draw_ui(vertex_2d windowSize);
    void draw_mode_select_ui();
    void draw_line_mod_ui();
    void draw_sector_mod_ui();
    void draw_texture_select_ui();

    void terminate_imgui();

    // mouse checkers for manipulating with drawing

    vertex_2d getMousePos(int x, int y);
    vertex_2d getSnapToGridPos(float x, float y);
    vertex_2d getMouseShift();
    vertex_2d applyMouseShift();

    //

    editorView view;
    Grid grid;
    vertex_2d windowSize;

    //
    
    vertex_2d prevMousePos;
    vertex_2d curMousePos;
    vertex_2d prevGridMousePos;
    vertex_2d curGridMousePos;

    vertex_2d gridOffset;

    SDL_Window* window;
    ImGuiIO& io;

    float zoom;

    bool prevLeftMouse;
    bool curLeftMouse;
  };

  
  
}