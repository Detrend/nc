#pragma once

#include <glad/glad.h>

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
    void init_imgui(SDL_Window* window, void* gl_context);
    void draw_ui(vertex_2d windowSize);
    void create_bottom_bar(vertex_2d& windowSize);
    void init_cursor_gl();
    void update_cursor_gl();
    void draw_cursor();
    void create_menu_bar();
    void terminate_imgui();

    void draw_map();

    void get_mouse_input();
    void get_left_mouse_button(Uint32 mouseState);
    vertex_2d get_mouse_pos(int x, int y);
    vertex_2d get_snap_to_grid_pos(float x, float y);
    vertex_2d get_snap_to_grid_pos();

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

    vertex_3d onGridPoint[2];

    GLuint vertexBuffer;
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint shaderProgram;
    GLuint vertexArrayBuffer;
    glm::mat4 viewMatrix;

    std::vector<MapPoint> mapPoints; // A TEMPORARY VARIABLE FOR TESTING
  };

  
  
}