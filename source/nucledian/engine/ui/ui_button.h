#pragma once
#include <engine/ui/ui_texture.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/resources/shader_program.h>
#include <engine/graphics/resources/texture.h>
#include <engine/game/game_system.h>

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

  class UiButton
  {
  public:
    UiButton();
    UiButton(const char* texture_name, vec2 position, vec2 scale, std::function<void(void)> func);
    bool is_point_in_rec(vec2 point);

    vec2 get_position();
    vec2 get_scale();

    void set_hover(bool hover);

    virtual void on_click();

    virtual void draw(ShaderProgramHandle button_material); // Draw takes the shader to modify its uniforms

  protected:
    const char* texture_name;
    vec2 position;
    vec2 scale;
    bool isHover = false;

  private:
    std::function<void(void)> func;
  };
  //======================================================================================
  
  class UiLoadGameButton : public UiButton
  {
  public:
    UiLoadGameButton(nc::GameSystem::SaveDbEntry& save_entry, vec2 position, vec2 scale);

    void on_click() override;
    void draw(ShaderProgramHandle digit_material);

  private:
    nc::GameSystem::SaveDbEntry& save;
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

    void go_back();
    void level_1_func();
    void level_2_func();
    void level_3_func();

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

    void laod_settings();
    void save_settings();
    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, GLuint VAO);
  private:

    const ShaderProgramHandle digit_shader;

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

    UiButton* sound_text = nullptr;
    UiButton* music_text = nullptr;
    UiButton* sensitivity_text = nullptr;
    UiButton* fullscreen_button = nullptr;
    UiButton* windowed_button = nullptr;
    UiButton* crosshair_text = nullptr;

    UiButton* sound_volume_less = nullptr;
    UiButton* sound_volume_more = nullptr;
    UiButton* music_volume_less = nullptr;
    UiButton* music_volume_more = nullptr;
    UiButton* sensitivity_less = nullptr;
    UiButton* sensitivity_more = nullptr;
    UiButton* crosshair_less = nullptr;
    UiButton* crosshair_more = nullptr;

    UiButton* go_back_button = nullptr;

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

    void update_saves();
    void update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse);
    void draw(ShaderProgramHandle button_material, ShaderProgramHandle digit_material, GLuint VAO);
  private:
    void go_back();
    void page_up();
    void page_down();

    const s32 PAGE_SIZE = 8;

    s32 page = 0;

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
    void yes_func();
    void no_func();

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
    void set_kill_stats(u32 enemies, u32 kills);
    void draw_stats(ShaderProgramHandle digit_material, GLuint VAO);
    void draw_kill_count(ShaderProgramHandle digit_material);
  private:

    void next_level_func();
    void do_nothing() {}
    void menu_func();

    UiButton* kills_text;
    UiButton* level_text;
    UiButton* demo_text;
    UiButton* completed_text;
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

    
    void set_page(MenuPages page);
    void set_transition_screen(bool enabled);
    void set_visible(bool visibility);
    void update();
    void draw();
    void post_init();
    void on_exit();
    void update_saves();

  private:
    void draw_cursor();
    vec2 get_normalized_mouse_pos();

    const ShaderProgramHandle button_material;
    const ShaderProgramHandle digit_material;

    MainMenuPage* main_menu_page = nullptr;
    OptionsPage* options_page = nullptr;
    LoadGamePage* load_game_page = nullptr;
    NewGamePage* new_game_page = nullptr;
    QuitGamePage* quit_game_page = nullptr;
    NextLevelPage* next_level_page = nullptr;

    MenuPages current_page = MenuPages::MAIN;

    bool visible = false;
    bool isTransition = false;

    bool cur_esc_pressed = false;
    bool prev_esc_pressed = false;

    uint32 prev_mousestate = 0;
    uint32 cur_mousestate = 0;

    GLuint VBO;
    GLuint VAO;
  };
}