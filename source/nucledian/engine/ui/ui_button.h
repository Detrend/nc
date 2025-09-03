#include <engine/ui/ui_texture.h>
#include <glad/glad.h>
#include <engine/graphics/graphics_system.h>

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
    UiButton(GuiTexture* texture, std::function<void(void)> func);
    bool is_point_in_rec(vec2 point);


  private:
    GuiTexture* texture;
    bool isHover = false;
    std::function<void(void)> func;
  };

  class MainMenuPage
  {
  public:
    MainMenuPage();
    ~MainMenuPage();

  private:
    UiButton* new_game_button = nullptr;
    UiButton* options_button = nullptr;
    UiButton* load_button = nullptr;
    UiButton* save_button = nullptr;
    UiButton* quit_button = nullptr;
  };

  class NewGamePage
  {
    
  };

  class OptionsPage
  {

  };

  class SaveGamePage 
  {

  };

  class LoadGamePage
  {

  };

  class MenuManager
  {
  public:
    MenuManager();
    ~MenuManager();
    
    vec2 get_normalized_mouse_pos();

    void set_visible(bool visibility);
    void draw();

  private:
    MainMenuPage* main_menu_page = nullptr;
    OptionsPage* options_page = nullptr;
    LoadGamePage* load_game_page = nullptr;
    SaveGamePage* save_game_page = nullptr;

    MenuPages current_page = MenuPages::MAIN;

    bool visible = false;
  };
}