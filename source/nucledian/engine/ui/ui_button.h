#include <engine/ui/ui_texture.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/shaders/shaders.h>
#include <engine/graphics/resources/material.h>
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
    SAVE
  };

  class UiButton
  {
  public:
    UiButton(const char* texture_name, vec2 position, vec2 scale, std::function<void(void)> func);
    bool is_point_in_rec(vec2 point);

    vec2 get_position();
    vec2 get_scale();

    void draw(MaterialHandle button_material); // Draw takes the shader to modify its uniforms

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

    void draw(MaterialHandle button_material, GLuint VAO);

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
    void draw();
  private:
  };

  class OptionsPage
  {
  public:
    void draw();
  private:
  };

  class SaveGamePage 
  {
  public:
    void draw();
  private:
  };

  class LoadGamePage
  {
  public:
    void draw();
  private:
  };

  class MenuManager
  {
  public:
    MenuManager();
    ~MenuManager();
    
    vec2 get_normalized_mouse_pos();

    void set_visible(bool visibility);
    void update();
    void draw();

  private:
    const MaterialHandle button_material;

    MainMenuPage* main_menu_page = nullptr;
    OptionsPage* options_page = nullptr;
    LoadGamePage* load_game_page = nullptr;
    SaveGamePage* save_game_page = nullptr;

    MenuPages current_page = MenuPages::MAIN;

    bool visible = false;

    bool cur_esc_pressed = false;
    bool prev_esc_pressed = false;

    GLuint VBO;
    GLuint VAO;
  };
}