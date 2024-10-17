#pragma once

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>

#include <engine/editor/map_info.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_opengl3.h>

#include <SDL.h>

#include <SDL_opengl.h>
//#define GL_GLEXT_PROTOTYPES

#include <SDL_opengl_glext.h>



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
    ~Grid();

  private:  
    std::vector<vertex_3d> points;
    // definition of the lines themeself

    GLuint vertex_buffer;
  };

  class EditorSystem : public IEngineModule
  {
  public:
    EditorSystem();
    static EngineModuleId get_module_id();  
    void on_event(ModuleEvent& event) override;
    ~EditorSystem();

    bool init(SDL_Window* window, void* gl_context);
    
    void render_grid();
    void render_map_2d();

    void destroy();

  private:
    // imgui skeleton
    void draw_ui();
    void draw_mode_select_ui();
    void draw_line_mod_ui();
    void draw_sector_mod_ui();
    void draw_texture_select_ui();

    void terminate_imgui();

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