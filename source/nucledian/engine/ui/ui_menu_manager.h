#pragma once
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/resources/shader_program.h>
#include <engine/graphics/resources/texture.h>
#include <engine/game/game_system.h>
#include <engine/ui/ui_button.h>
#include <engine/ui/ui_menu_page.h>

#include <vector>
#include <functional>
#include <string>

namespace nc
{
  class MenuManager
  {
  public:
    MenuManager();
    ~MenuManager();

    // determines if a transition screen should be rendered instead
    void set_transition_screen(bool enabled);

    // determines visibility of menu
    void set_visible(bool visibility);

    void update();
    void draw();

    void post_init();

    void on_exit();

    // tells LoadGamePage to update saves
    void update_saves();

    void set_page(MenuPages page);

  private:
    void draw_cursor();

    // return position of mouse in (-1, 1) coordinates
    vec2 get_normalized_mouse_pos();

    //shaders
    const ShaderProgramHandle button_material;
    const ShaderProgramHandle digit_material;

    // pages
    MainMenuPage* main_menu_page = nullptr;
    OptionsPage* options_page = nullptr;
    LoadGamePage* load_game_page = nullptr;
    NewGamePage* new_game_page = nullptr;
    QuitGamePage* quit_game_page = nullptr;
    NextLevelPage* next_level_page = nullptr;

    MenuPages current_page = MenuPages::MAIN;

    // properties
    bool visible = false;
    bool isTransition = false;

    // input variables
    bool cur_esc_pressed = false;
    bool prev_esc_pressed = false;

    uint32 prev_mousestate = 0;
    uint32 cur_mousestate = 0;

    // openGL properties
    GLuint VBO;
    GLuint VAO;
  };
}