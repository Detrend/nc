// Project Nuclidean Source File
#include <engine/ui/user_interface_system.h>

#include <engine/graphics/resources/res_lifetime.h>

#include <engine/ui/ui_menu_manager.h>
#include <SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <engine/game/game_system.h>
#include <engine/game/game_helpers.h>
#include <engine/core/engine.h>
#include <engine/core/module_event.h>
#include <engine/graphics/shaders/shaders.h>

namespace nc
{

  //============================================================================================

  MenuManager::MenuManager() :
    button_material(shaders::ui_button::VERTEX_SOURCE, shaders::ui_button::FRAGMENT_SOURCE),
    digit_material(shaders::ui_text::VERTEX_SOURCE, shaders::ui_text::FRAGMENT_SOURCE)
  {

    main_menu_page = new MainMenuPage();
    options_page = new OptionsPage();
    load_game_page = new LoadGamePage();
    new_game_page = new NewGamePage();
    quit_game_page = new QuitGamePage();
    next_level_page = new NextLevelPage();

    // Create VAO and VBO
    vec2 vertices[] = { vec2(-1.0f, 1.0f), vec2(0.0f, 0.015f),
      vec2(-1.0f, -1.0f), vec2(0.0f, 1.0f - 0.015f),
      vec2(1.0f, 1.0f), vec2(1.0f, 0.015f),
      vec2(1.0f, -1.0f), vec2(1.0f, 1.0f - 0.015f) };

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, &vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, &vertices, GL_STATIC_DRAW);
    // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    // uvs
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  //============================================================================================

  void MenuManager::post_init()
  {
    options_page->laod_settings();
  }

  //=============================================================================================

  MenuManager::~MenuManager()
  {
    delete main_menu_page;
    delete options_page;
    delete load_game_page;
    delete new_game_page;
    delete next_level_page;

    glDeleteBuffers(1, &VAO);
    glDeleteBuffers(1, &VBO);
  }

  //==============================================================================================

  void MenuManager::set_page(MenuPage page)
  {
    current_page = page;
  }

  //=============================================================================================

  void MenuManager::on_exit()
  {
    options_page->save_settings();
  }

  //=============================================================================================

  vec2 MenuManager::get_normalized_mouse_pos()
  {
    int mouse_x, mouse_y;

    uint32 state = SDL_GetMouseState(&mouse_x, &mouse_y);

    prev_mousestate = cur_mousestate;
    cur_mousestate = state;

    vec2 dimensions = get_engine().get_module<GraphicsSystem>().get_window_size();

    f32 width = dimensions.x;
    f32 height = dimensions.y;

    f32 half_width = width / 2.0f;
    f32 half_height = height / 2.0f;

    //normalize between <0, 2>, then minus one to move it to <-1, 1>
    f32 norm_x = mouse_x / half_width - 1.0f;
    f32 norm_y = mouse_y / half_height - 1.0f;

    return vec2(norm_x, -norm_y);
  }

  //=============================================================================================
  void MenuManager::set_transition_screen([[maybe_unused]]bool enabled)
  {
    isTransition = enabled;
    if (isTransition)
    {
      [[maybe_unused]] f64 time = GameHelpers::get().get_time_since_start(); // time stat is something we might want to add in the future
      u32 enemies = get_engine().get_module<GameSystem>().get_enemy_count();
      u32 kills = get_engine().get_module<GameSystem>().get_kill_count();
      next_level_page->set_kill_stats(enemies, kills);
    }
  }

  //=============================================================================================

  void MenuManager::set_visible(bool visibility)
  {
    if (visible != visibility)
    {
      namespace EventType = ModuleEventType;

      get_engine().send_event(ModuleEvent
      {
        .type = visibility ? EventType::menu_opened : EventType::menu_closed
      });
    }

    visible = visibility;
    current_page = MenuPage::main_page;
  }

  //===============================================================================================

  void MenuManager::update()
  {
    // transition screen?
    if (isTransition)
    {
      vec2 mouse_pos = get_normalized_mouse_pos();
      next_level_page->update(mouse_pos, prev_mousestate, cur_mousestate);
      return;
    }

    // ESC pressed?
    prev_esc_pressed = cur_esc_pressed;
    cur_esc_pressed = false;

    int size;
    const Uint8* keyboard = SDL_GetKeyboardState(&size);
    if (keyboard[SDL_SCANCODE_ESCAPE])
    {
      cur_esc_pressed = true;
    }

    bool pressed = cur_esc_pressed && !prev_esc_pressed;
    bool locked  = get_engine().is_menu_locked_visible();

    // check if we want to change menu visibility or just swap to main menu
    if (pressed)
    {
      if (current_page != MenuPage::main_page)
      {
        // Go back to menu
        set_page(MenuPage::main_page);
      }
      else if (!locked)
      {
        // Change visibility
        set_visible(!visible);
      }
    }

    if (!visible) {
      return;
    }

    vec2 mouse_pos = get_normalized_mouse_pos();

    // call to render current page itself
    switch (current_page)
    {
    case MenuPage::main_page:
      main_menu_page->update(mouse_pos, prev_mousestate, cur_mousestate);
      break;
    case MenuPage::new_game:
      new_game_page->update(mouse_pos, prev_mousestate, cur_mousestate);
      break;
    case MenuPage::options:
      options_page->update(mouse_pos, prev_mousestate, cur_mousestate);
      break;
    case MenuPage::load:
      load_game_page->update(mouse_pos, prev_mousestate, cur_mousestate);
      break;
    case MenuPage::save:
      break;
    case MenuPage::quit:
      quit_game_page->update(mouse_pos, prev_mousestate, cur_mousestate);
      break;
    default:
      break;
    }
  }

  //============================================================================================

  void MenuManager::draw()
  {
    // transition?
    if (isTransition)
    {
      next_level_page->draw(button_material, digit_material, VAO);
      draw_cursor();
      return;
    }

    if (!visible)
    {
      return;
    }

    switch (current_page)
    {
    case MenuPage::main_page:
      main_menu_page->draw(button_material, VAO);
      break;
    case MenuPage::new_game:
      new_game_page->draw(button_material, VAO);
      break;
    case MenuPage::options:
      options_page->draw(button_material, VAO);
      break;
    case MenuPage::load:
      load_game_page->draw(button_material, digit_material, VAO);
      break;
    case MenuPage::save:
      break;
    case MenuPage::quit:
      quit_game_page->draw(button_material, VAO);
      break;
    default:
      break;
    }

    draw_cursor();
  }

  //==============================================================================================

  void MenuManager::draw_cursor()
  {
    // shader init
    button_material.use();

    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    vec2 pos = get_normalized_mouse_pos();

    // texture
    const char* cursor_tex = "ui_cursor";

    const TextureManager& manager = TextureManager::get();
    const TextureHandle& texture = manager[cursor_tex];

    glActiveTexture(GL_TEXTURE0);

    // getting shader uniforms
    glm::mat4 trans_mat = glm::mat4(1.0f);
    vec2 translate = pos + vec2(0.015, -0.02);
    trans_mat = glm::translate(trans_mat, glm::vec3(translate.x, translate.y, 0));
    trans_mat = glm::scale(trans_mat, glm::vec3(0.015f, 0.02f, 1));

    const glm::mat4 final_trans = trans_mat;

    // setting shader uniforms
    button_material.set_uniform(shaders::ui_button::TRANSFORM, final_trans);
    button_material.set_uniform(shaders::ui_button::ATLAS_SIZE, texture.get_atlas().get_size());
    button_material.set_uniform(shaders::ui_button::TEXTURE_POS, texture.get_pos());
    button_material.set_uniform(shaders::ui_button::TEXTURE_SIZE, texture.get_size());
    button_material.set_uniform(shaders::ui_button::HOVER, false);

    // draw
    glBindTexture(GL_TEXTURE_2D, texture.get_atlas().handle);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // shader unbinding

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindVertexArray(0);
  }

  //=============================================================================================


  void MenuManager::update_saves()
  {
    load_game_page->update_saves();
  }

  //==============================================================================================
}