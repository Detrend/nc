#include <engine/ui/ui_button.h>
#include <SDL.h>

namespace nc
{
  UiButton::UiButton(GuiTexture* texture, std::function<void(void)> func)
  {
    this->texture = texture;
    this->func = func;
  }

  bool UiButton::is_point_in_rec(vec2 point)
  {
    vec2 position = texture->get_position();
    vec2 scale = texture->get_scale();

    vec2 start = position - scale / 2.0f;
    vec2 end = position + scale / 2.0f;

    if (point.x < start.x || point.x > end.x)
    {
      return false;
    }

    if (point.y < start.y || point.y > end.y)
    {
      return false;
    }

    return true;
  }

  //============================================================================================

  MenuManager::MenuManager()
  {
    main_menu_page = new MainMenuPage();
    options_page = new OptionsPage();
    load_game_page = new LoadGamePage();
    save_game_page = new SaveGamePage();
  }

  //============================================================================================

  MenuManager::~MenuManager()
  {
    delete main_menu_page;
    delete options_page;
    delete load_game_page;
    delete save_game_page;
  }

  //=============================================================================================

  vec2 MenuManager::get_normalized_mouse_pos()
  {
    int mouse_x, mouse_y;

    SDL_GetMouseState(&mouse_x, &mouse_y);

    f32 width = GraphicsSystem::WINDOW_WIDTH;
    f32 height = GraphicsSystem::WINDOW_HEIGHT;

    f32 half_width = width / 2.0f;
    f32 half_height = height / 2.0f;

    //normalize between <0, 2>, then minus one to move it to <-1, 1>
    f32 norm_x = mouse_x / half_width - 1.0f;
    f32 norm_y = mouse_y / half_height - 1.0f;

    return vec2(norm_x, - norm_y);
  }

  //=============================================================================================

  void MenuManager::set_visible(bool visibility)
  {
    visible = visibility;
    current_page = MAIN;
  }

  //============================================================================================

  void MenuManager::draw()
  {
  }

  //===========================================================================================

  MainMenuPage::MainMenuPage()
  {
  }

  //============================================================================================

  MainMenuPage::~MainMenuPage()
  {
  }
}