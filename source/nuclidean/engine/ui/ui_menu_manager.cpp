// Project Nuclidean Source File
#include <engine/ui/ui_menu_manager.h>

#include <SDL.h>

#include <glm/gtc/matrix_transform.hpp>

#include <engine/core/engine.h>
#include <engine/core/module_event.h>
#include <engine/graphics/ui_draw_list.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/resources/texture.h>

#include <logging.h>


namespace nc
{

//============================================================================================
MenuManager::MenuManager()
{
  main_menu_page = new MainMenuPage();
  options_page = new OptionsPage();
  load_game_page = new LoadGamePage();
  new_game_page = new NewGamePage();
  quit_game_page = new QuitGamePage();
  next_level_page = new NextLevelPage();
}

//============================================================================================
void MenuManager::post_init()
{
  options_page->load_settings();
}

//=============================================================================================
MenuManager::~MenuManager()
{
  delete main_menu_page;
  delete options_page;
  delete load_game_page;
  delete new_game_page;
  delete next_level_page;
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
  prev_mousestate = cur_mousestate;
  cur_mousestate  = SDL_GetMouseState(nullptr, nullptr);

  return query_normalized_mouse_pos();
}

//=============================================================================================
vec2 MenuManager::query_normalized_mouse_pos()
{
  int mouse_x, mouse_y;
  SDL_GetMouseState(&mouse_x, &mouse_y);

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
void MenuManager::set_transition_screen(bool enabled)
{
  is_transition = enabled;
  if (is_transition)
  {
    // [[maybe_unused]] f64 time = GameHelpers::get().get_time_since_start(); // time stat is something we might want to add in the future
    u32 enemies = get_engine().get_module<GameSystem>().get_enemy_count();
    u32 kills = get_engine().get_module<GameSystem>().get_kill_count();   
    next_level_page->set_kill_stats(enemies, kills);

    u32 secrets = get_engine().get_module<GameSystem>().get_secret_count();
    u32 revealed = get_engine().get_module<GameSystem>().get_revealed_count();
    next_level_page->set_secret_stats(secrets, revealed);
  }
}

//=============================================================================================
void MenuManager::set_visible(bool visibility)
{
  if (is_visible != visibility)
  {
    namespace EventType = ModuleEventType;

    get_engine().send_event(ModuleEvent
    {
      .type = visibility ? EventType::menu_opened : EventType::menu_closed
    });
  }

  is_visible = visibility;
  current_page = MenuPage::main_page;
}

//=============================================================================================
bool MenuManager::get_is_visible() const 
{
  return is_visible;
}

//=============================================================================
bool MenuManager::set_presentation_slides
(
  const std::vector<std::string>& slide_texture_names
)
{
  presentation_slides.clear();
  presentation_index = 0;

  const TextureMap& textures =
    TextureManager::get().get_atlas_bundle(ResLifetime::Game).textures;

  for (const std::string& name : slide_texture_names)
  {
    if (textures.contains(name))
    {
      presentation_slides.push_back(name);
    }
    else
    {
      nc_warn("Presentation slide texture \"{}\" not found, skipping.", name);
    }
  }

  is_presentation = !presentation_slides.empty();
  return is_presentation;
}

//=============================================================================
void MenuManager::update_presentation()
{
  if (presentation_slides.empty())
  {
    return;
  }

  int          num_keys = 0;
  const Uint8* keyboard = SDL_GetKeyboardState(&num_keys);

  const bool left  = keyboard[SDL_SCANCODE_LEFT];
  const bool right = keyboard[SDL_SCANCODE_RIGHT];

  const size_t count = presentation_slides.size();

  // Edge-triggered: one slide change per key press.
  if (left && !prev_left_pressed)
  {
    presentation_index = (presentation_index + count - 1) % count;
  }
  else if (right && !prev_right_pressed)
  {
    presentation_index = (presentation_index + 1) % count;
  }

  prev_left_pressed  = left;
  prev_right_pressed = right;
}

//===============================================================================================
void MenuManager::update()
{
  // presentation mode?
  if (is_presentation)
  {
    update_presentation();
    return;
  }

  // transition screen?
  if (is_transition)
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
      set_visible(!is_visible);
    }
  }

  if (!is_visible) {
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
void MenuManager::draw(UiDrawList& out) const
{
  // presentation mode?
  if (is_presentation)
  {
    draw_presentation(out);
    return;
  }

  // transition?
  if (is_transition)
  {
    next_level_page->draw(out);
    draw_cursor(out);
    return;
  }

  if (!is_visible)
  {
    return;
  }

  switch (current_page)
  {
  case MenuPage::main_page:
    main_menu_page->draw(out);
    break;
  case MenuPage::new_game:
    new_game_page->draw(out);
    break;
  case MenuPage::options:
    options_page->draw(out);
    break;
  case MenuPage::load:
    load_game_page->draw(out);
    break;
  case MenuPage::save:
    break;
  case MenuPage::quit:
    quit_game_page->draw(out);
    break;
  default:
    break;
  }

  draw_cursor(out);
}

//==============================================================================================
void MenuManager::draw_cursor(UiDrawList& out) const
{
  vec2 pos = query_normalized_mouse_pos();

  const TextureHandle& texture = TextureManager::get()["ui_cursor"];
  out.add_image(texture, pos + vec2(0.015f, -0.02f), vec2(0.015f, 0.02f));
}

//=============================================================================================
void MenuManager::draw_presentation(UiDrawList& out) const
{
  if (presentation_slides.empty())
  {
    return;
  }

  // Slide names are validated in set_presentation_slides, so this lookup is safe
  const TextureHandle& texture = TextureManager::get()[presentation_slides[presentation_index]];

  // full-screen quad
  out.add_image(texture, VEC2_ZERO, VEC2_ONE);
}

//=============================================================================================
void MenuManager::update_saves()
{
  load_game_page->update_saves();
}

}
