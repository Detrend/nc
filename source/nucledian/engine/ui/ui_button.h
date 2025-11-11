#pragma once
#include <engine/ui/ui_texture.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/shaders/shaders.h>
#include <engine/graphics/resources/shader_program.h>
#include <engine/graphics/resources/texture.h>

#include <vector>
#include <functional>

namespace nc
{
  enum MenuPages
  {
    MAIN,
    NEW_GAME,
    OPTIONS,
    LOAD,
    SAVE,
    QUIT
  };

  class UiButton
  {
  public:
    UiButton(const char* texture_name, vec2 position, vec2 scale, std::function<void(void)> func);
    bool is_point_in_rec(vec2 point);

    vec2 get_position();
    vec2 get_scale();

    void set_hover(bool hover);

    void on_click();

    void draw(ShaderProgramHandle button_material); // Draw takes the shader to modify its uniforms

  private:
    const char* texture_name;
    vec2 position;
    vec2 scale;
    bool isHover = false;
    std::function<void(void)> func;
  };

  class MainMenuPage
  {
  public:
    MainMenuPage();
    ~MainMenuPage();

    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, GLuint VAO);

  private:

    //These functions are for buttons
    void new_game_func();
    void options_func();
    void load_game_func();
    void save_game_func();
    void quit_func();

    UiButton* new_game_button = nullptr;
    UiButton* options_button = nullptr;
    UiButton* load_button = nullptr;
    UiButton* save_button = nullptr;
    UiButton* quit_button = nullptr;
  };

  class NewGamePage
  {
  public:
    NewGamePage();
    ~NewGamePage();

    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, GLuint VAO);
  private:

    void level_1_func();
    void level_2_func();
    void level_3_func();

    UiButton* level_1_button = nullptr;
    UiButton* level_2_button = nullptr;
    UiButton* level_3_button = nullptr;
  };

  class OptionsPage
  {
  public:
    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, GLuint VAO);
  private:
  };

  class SaveGamePage 
  {
  public:
    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, GLuint VAO);
  private:
  };

  class LoadGamePage
  {
  public:
    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, GLuint VAO);
  private:
  };

  class QuitGamePage
  {
  public:
    QuitGamePage();
    ~QuitGamePage();

    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, GLuint VAO);
  private:
    void yes_func();
    void no_func();

    UiButton* yes_button = nullptr;
    UiButton* no_button = nullptr;
  };

  class MenuManager
  {
  public:
    MenuManager();
    ~MenuManager();

    void set_page(MenuPages page);

    vec2 get_normalized_mouse_pos();

    void set_visible(bool visibility);
    void update();
    void draw();
    void draw_cursor();

  private:
    const ShaderProgramHandle button_material;

    MainMenuPage* main_menu_page = nullptr;
    OptionsPage* options_page = nullptr;
    LoadGamePage* load_game_page = nullptr;
    NewGamePage* new_game_page = nullptr;
    QuitGamePage* quit_game_page = nullptr;

    MenuPages current_page = MenuPages::MAIN;

    bool visible = false;

    bool cur_esc_pressed = false;
    bool prev_esc_pressed = false;

    uint32 prev_mousestate = 0;
    uint32 cur_mousestate = 0;

    GLuint VBO;
    GLuint VAO;
  };
}