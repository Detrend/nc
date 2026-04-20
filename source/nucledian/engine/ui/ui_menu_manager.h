#pragma once
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/resources/shader_program.h>
#include <engine/graphics/resources/texture.h>
#include <engine/game/game_system.h>
#include <engine/ui/ui_button.h>

#include <vector>
#include <functional>
#include <string>

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
  
  //======================================================================================

  class MainMenuPage
  {
  public:
    MainMenuPage();
    ~MainMenuPage();

    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);

    void draw(ShaderProgramHandle button_material, GLuint VAO);

  private:
    //These methods are for buttons
    void new_game_func();
    void options_func();
    void load_game_func();
    void save_game_func();
    void quit_func();

    // buttons
    UiButton* new_game_button = nullptr;
    UiButton* options_button = nullptr;
    UiButton* load_button = nullptr;
    UiButton* save_button = nullptr;
    UiButton* quit_button = nullptr;

    // text
    UiButton* nuclidean_text = nullptr;
  };

  //======================================================================================

  class NewGamePage
  {
  public:
    NewGamePage();
    ~NewGamePage();

    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, GLuint VAO);
  private:
    // methods for buttons
    void go_back();
    void level_1_func();
    void level_2_func();
    void level_3_func();

    // buttons
    UiButton* go_back_button = nullptr;
    UiButton* level_1_button = nullptr;
    UiButton* level_2_button = nullptr;
    UiButton* level_3_button = nullptr;
  };

  //======================================================================================

  class OptionsPage
  {
  public:
    OptionsPage();
    ~OptionsPage();

    // Reads "settings.cfg" and changes settings accordingly
    void laod_settings();

    // Saves settings to "settings.cfg"
    void save_settings();

    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, GLuint VAO);
  private:
    // shader for writing numbers
    const ShaderProgramHandle digit_shader;

    // methods for buttons
    void do_nothing() {};

    void set_windowed();
    void set_fullscreen();

    void set_sensitivity_less();
    void set_sensitivity_more();

    void set_sound_less();
    void set_sound_more();

    void set_music_less();
    void set_music_more();

    void set_crosshair_less();
    void set_crosshair_more();

    void go_back();

    // text
    UiButton* sound_text = nullptr;
    UiButton* music_text = nullptr;
    UiButton* sensitivity_text = nullptr;
    UiButton* fullscreen_button = nullptr;
    UiButton* windowed_button = nullptr;
    UiButton* crosshair_text = nullptr;

    // buttons
    UiButton* sound_volume_less = nullptr;
    UiButton* sound_volume_more = nullptr;
    UiButton* music_volume_less = nullptr;
    UiButton* music_volume_more = nullptr;
    UiButton* sensitivity_less = nullptr;
    UiButton* sensitivity_more = nullptr;
    UiButton* crosshair_less = nullptr;
    UiButton* crosshair_more = nullptr;

    UiButton* go_back_button = nullptr;

    // settings properties
    bool isWindowed = true;

    int soundStep = 9;
    int musicStep = 9;
    int sensitivityStep = 4;
    int crosshairStep = 1;
  };

  //======================================================================================

  /*class SaveGamePage
  {
  public:
    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, GLuint VAO);
  private:
  };*/

  //======================================================================================

  class LoadGamePage
  {
  public:
    LoadGamePage();
    ~LoadGamePage();

    // read saves from game system and create buttons for saves
    void update_saves();

    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, ShaderProgramHandle digit_material, GLuint VAO);
  private:
    //methods for buttons
    void go_back();
    void page_up();
    void page_down();

    // consts
    const s32 PAGE_SIZE = 8;

    // page to determine which buttons to draw
    s32 page = 0;

    // buttons
    UiButton* go_back_button = nullptr;
    UiButton* page_up_button = nullptr;
    UiButton* page_down_button = nullptr;

    std::vector<UiLoadGameButton*> load_game_buttons;
  };

  //======================================================================================

  class QuitGamePage
  {
  public:
    QuitGamePage();
    ~QuitGamePage();

    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, GLuint VAO);
  private:
    //methods for buttons
    void yes_func();
    void no_func();

    // buttons
    UiButton* yes_button = nullptr;
    UiButton* no_button = nullptr;
  };

  //======================================================================================

  class NextLevelPage
  {
  public:
    NextLevelPage();
    ~NextLevelPage();

    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, ShaderProgramHandle digit_material, GLuint VAO);

    // set values to be rendered
    void set_kill_stats(u32 enemies, u32 kills);

  private:
    // draw the stats (kills so far)
    void draw_stats(ShaderProgramHandle digit_material, GLuint VAO);

    // draw the number of kills itself
    void draw_kill_count(ShaderProgramHandle digit_material);

    //methods for buttons
    void next_level_func();
    void do_nothing() {}
    void menu_func();

    // text
    UiButton* kills_text;
    UiButton* level_text;
    UiButton* demo_text;
    UiButton* completed_text;

    // button
    UiButton* next_level_button;
    UiButton* menu_button;

    u32 enemy_count = 0;
    u32 kill_count = 0;
  };

  //======================================================================================

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