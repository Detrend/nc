#include <engine/graphics/resources/res_lifetime.h>

#include <engine/ui/ui_button.h>
#include <SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace nc
{
  UiButton::UiButton(const char* texture_name, vec2 position, vec2 scale, std::function<void(void)> func)
  {
    this->texture_name = texture_name;

    this->position = position;
    this->scale = scale;
    this->func = func;

  }

  //================================================================================================

  bool UiButton::is_point_in_rec(vec2 point)
  {
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

  //===================================================================================================

  vec2 UiButton::get_position()
  {
    return position;
  }

  //======================================================================================================

  vec2 UiButton::get_scale()
  {
    return scale;
  }

  //====================================================================================================

  void UiButton::draw([[maybe_unused]] const MaterialHandle button_material)
  {
    const TextureManager& manager = TextureManager::get();
    [[maybe_unused]] const TextureHandle& texture = manager[texture_name];

    glm::mat4 trans_mat = glm::mat4(1.0f);
    vec2 translate = position;
    trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
    trans_mat = glm::scale(trans_mat, glm::vec3(scale.x, scale.y, 1));

    const glm::mat4 final_trans = trans_mat;

    // button_material.set_uniform(shaders::ui_button::TRANSFORM, final_trans);

    glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

  //============================================================================================

  MenuManager::MenuManager() : 
  button_material(shaders::ui_button::VERTEX_SOURCE, shaders::ui_button::FRAGMENT_SOURCE)
  {

    main_menu_page = new MainMenuPage();
    options_page = new OptionsPage();
    load_game_page = new LoadGamePage();
    save_game_page = new SaveGamePage();

    vec2 vertices[] = { vec2(-1, 1), vec2(-1, -1), vec2(1, 1), vec2(1, -1) };

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 2, &vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 2, &vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

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

  //===============================================================================================

  void MenuManager::update()
  {
    prev_esc_pressed = cur_esc_pressed;
    cur_esc_pressed = false;

    int size;
    const Uint8* keyboard = SDL_GetKeyboardState(&size);
    if (keyboard[SDL_SCANCODE_ESCAPE])
    {
      cur_esc_pressed = true;
    }

    if (cur_esc_pressed && !prev_esc_pressed)
    {
      set_visible(!visible);
    }

    if (!visible) {
      return;
    }

    switch (current_page)
    {
    case nc::MAIN:
      break;
    case nc::NEW_GAME:
      break;
    case nc::OPTIONS:
      break;
    case nc::LOAD:
      break;
    case nc::SAVE:
      break;
    default:
      break;
    }
  }

  //============================================================================================

  void MenuManager::draw()
  {
    switch (current_page)
    {
    case nc::MAIN:
      main_menu_page->draw(button_material, VAO);
      break;
    case nc::NEW_GAME:
      break;
    case nc::OPTIONS:
      break;
    case nc::LOAD:
      break;
    case nc::SAVE:
      break;
    default:
      break;
    }
  }

  //===========================================================================================

  MainMenuPage::MainMenuPage()
  {
  }

  //============================================================================================

  MainMenuPage::~MainMenuPage()
  {
  }

  //=============================================================================================

  void MainMenuPage::draw(MaterialHandle button_material, GLuint VAO)
  {
    button_material.use();

    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);

    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    new_game_button->draw(button_material);
    options_button->draw(button_material);
    load_button->draw(button_material);
    save_button->draw(button_material);
    quit_button->draw(button_material);


    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(0);
    glBindVertexArray(0);
  }

  //==============================================================================================

  void MainMenuPage::new_game_func()
  {
  }

  //==============================================================================================

  void MainMenuPage::options_func()
  {
  }

  //================================================================================================

  void MainMenuPage::load_game_func()
  {
  }

  //==============================================================================================

  void MainMenuPage::save_game_func()
  {
  }

  //================================================================================================

  void MainMenuPage::quit_func()
  {
  }

  //============================================================================================

  void NewGamePage::draw()
  {
  }

  //============================================================================================
  void OptionsPage::draw()
  {
  }

  //=============================================================================================
  void SaveGamePage::draw()
  {
  }

  //==============================================================================================

  void LoadGamePage::draw()
  {
  }
}