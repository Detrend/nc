// Project Nuclidean Source File
#include <engine/ui/ui_menu_page.h>
#include <engine/ui/user_interface_system.h>

#include <SDL.h>

#include <glm/gtc/matrix_transform.hpp>

#include <engine/game/game_system.h>
#include <engine/core/engine.h>
#include <engine/input/input_system.h>
#include <engine/sound/sound_system.h>
#include <engine/core/module_event.h>
#include <engine/graphics/ui_draw_list.h>
#include <engine/graphics/resources/texture.h>
#include <engine/graphics/graphics_system.h>
#include <engine/player/save_types.h>

#include <engine/sound/sound_resources.h>

#include <fstream>
#include <format>
#include <json/json.hpp>

namespace nc
{

NextLevelPage::NextLevelPage()
{
  next_level_button = new UiButton("ui_next_level", vec2(0.0f, -0.8f), vec2(0.3f, 0.066f), std::bind(&NextLevelPage::next_level_func, this));
  menu_button = new UiButton("ui_menu", vec2(0.0f, -0.8f), vec2(0.12f, 0.066f), std::bind(&NextLevelPage::menu_func, this));
  kills_text = new UiButton("ui_kills", vec2(-0.3f, 0.3f), vec2(0.30f, 0.066f), std::bind(&NextLevelPage::do_nothing, this));
  secrets_text = new UiButton("ui_secrets", vec2(-0.3f, 0.1f), vec2(0.30f, 0.066f), std::bind(&NextLevelPage::do_nothing, this));
  level_text = new UiButton("ui_level", vec2(0.0f, 0.8f), vec2(0.15f, 0.066f), std::bind(&NextLevelPage::do_nothing, this));
  demo_text = new UiButton("ui_demo", vec2(0.0f, 0.8f), vec2(0.12f, 0.066f), std::bind(&NextLevelPage::do_nothing, this));
  completed_text = new UiButton("ui_completed", vec2(0.0f, 0.6f), vec2(0.253f, 0.066f), std::bind(&NextLevelPage::do_nothing, this));
}

//=============================================================================================
NextLevelPage::~NextLevelPage()
{
  delete next_level_button;
  delete kills_text;
  delete demo_text;
  delete level_text;
  delete completed_text;
  delete menu_button;
  delete secrets_text;
}

//=============================================================================================
void NextLevelPage::update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse)
{
  // determines wheter next_level or back_to_menu button should be updated
  UiButton* hover_over_button = nullptr;

  if (get_engine().get_module<GameSystem>().get_level_name() != Levels::LEVEL_2)
  {
    next_level_button->set_hover(false);

    if (next_level_button->is_point_in_rec(mouse_pos))
    {
      hover_over_button = next_level_button;
    }
  }
  else
  {
    menu_button->set_hover(false);

    if (menu_button->is_point_in_rec(mouse_pos))
    {
      hover_over_button = menu_button;
    }
  }

  if (hover_over_button != nullptr)
  {
    hover_over_button->set_hover(true);

    if (!(prev_mouse & SDL_BUTTON(1)) && (cur_mouse & SDL_BUTTON(1)))
    {
      hover_over_button->on_click();
    }
  }
}

//=============================================================================================
void NextLevelPage::draw(UiDrawList& out) const
{
  // determines wheter next_level or back_to_menu button should be rendered
  kills_text->draw(out);
  secrets_text->draw(out);
  if (get_engine().get_module<GameSystem>().get_level_name() == Levels::LEVEL_2)
  {
    demo_text->draw(out);
    menu_button->draw(out);
  }
  else
  {
    level_text->draw(out);
    next_level_button->draw(out);
  }
  completed_text->draw(out);

  draw_stats(out);
}

//=============================================================================================
void NextLevelPage::set_kill_stats(u32 enemies, u32 kills)
{
  enemy_count = enemies;
  kill_count = kills;
}

void NextLevelPage::set_secret_stats(u32 secrets, u32 revealed)
{
  secret_count = secrets;
  revealed_count = revealed;
}

//=============================================================================================
void NextLevelPage::draw_stats(UiDrawList& out) const
{
  draw_ratio(out, vec2(0.5f, 0.3f), kill_count,     enemy_count);
  draw_ratio(out, vec2(0.5f, 0.1f), revealed_count, secret_count);
}

//=============================================================================================
void NextLevelPage::draw_ratio(UiDrawList& out, vec2 position, u32 count, u32 total)
{
  // renders numbers char by char from right to left
  vec2 pos_dif = vec2(-0.06f, 0.0f);
  vec2 scale   = vec2(0.0277f, 0.066f);

  const TextureHandle& font = TextureManager::get()["ui_font"];

  auto draw_number = [&](u32 display_count)
  {
    do
    {
      out.add_char(font, position, scale, cast<s32>(display_count % 10 + '0'));

      display_count = display_count / 10;
      position += pos_dif;
    } while (display_count > 0);
  };

  draw_number(total);

  // separating '/'
  UiQuad& slash = out.add_char(font, position, scale, '/');
  slash.texture_size -= vec2(1.0f, 0.0f);
  position += pos_dif;

  draw_number(count);
}

//=============================================================================================
void MainMenuPage::update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse)
{
  //update buttons and determine which (one) button can be pressed based on mouse position

  UiButton* hover_over_button = nullptr;

  new_game_button->set_hover(false);
  options_button->set_hover(false);
  load_button->set_hover(false);
  save_button->set_hover(false);
  quit_button->set_hover(false);

  if (new_game_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = new_game_button;
  }
  else if (options_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = options_button;
  }
  else if (load_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = load_button;
  }
  else if (save_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = save_button;
  }
  else if (quit_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = quit_button;
  }

  if (hover_over_button != nullptr)
  {
    hover_over_button->set_hover(true);

    if (!(prev_mouse & SDL_BUTTON(1)) && (cur_mouse & SDL_BUTTON(1)))
    {
      hover_over_button->on_click();
    }
  }
}

//==============================================================================================
void NewGamePage::update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse)
{
  //update buttons and determine which (one) button can be pressed based on mouse position
  UiButton* hover_over_button = nullptr;

  go_back_button->set_hover(false);
  level_1_button->set_hover(false);
  level_2_button->set_hover(false);
  //level_3_button->set_hover(false);

  if (level_1_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = level_1_button;
  }
  else if (level_2_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = level_2_button;
  }
  /*else if (level_3_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = level_3_button;
  }*/
  else if (go_back_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = go_back_button;
  }

  if (hover_over_button != nullptr)
  {
    hover_over_button->set_hover(true);

    if (!(prev_mouse & SDL_BUTTON(1)) && (cur_mouse & SDL_BUTTON(1)))
    {
      hover_over_button->on_click();
    }
  }
}

//==============================================================================================
void MainMenuPage::draw(UiDrawList& out) const
{
  new_game_button->draw(out);
  options_button->draw(out);
  load_button->draw(out);
  save_button->draw(out);
  quit_button->draw(out);
  nuclidean_text->draw(out);
}

//=============================================================================================
void NextLevelPage::next_level_func()
{
  get_engine().send_event(ModuleEvent
    {
      .type = ModuleEventType::next_level_requested
    });
}

//=============================================================================================
void NextLevelPage::menu_func()
{
  get_engine().send_event(ModuleEvent
    {
      .type = ModuleEventType::main_menu_requested
    });
}

//==============================================================================================
void MainMenuPage::new_game_func()
{
  get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MenuPage::new_game);
}

//==============================================================================================
void MainMenuPage::options_func()
{
  get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MenuPage::options);
}

//================================================================================================
void MainMenuPage::load_game_func()
{
  get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->update_saves();
  get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MenuPage::load);
}

//==============================================================================================
void MainMenuPage::save_game_func()
{
  GameSystem::get().save_game();
}

//================================================================================================
void MainMenuPage::quit_func()
{
  get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MenuPage::quit);
}

//============================================================================================
NewGamePage::NewGamePage()
{
  go_back_button = new UiButton("ui_back", vec2(0.0f, 0.3f), vec2(0.3f, 0.066f), std::bind(&NewGamePage::go_back, this));
  level_1_button = new UiButton("ui_level1", vec2(0.0f, 0.15f), vec2(0.3f, 0.066f), std::bind(&NewGamePage::level_1_func, this));
  level_2_button = new UiButton("ui_level2", vec2(0.0f, 0.0f), vec2(0.3f, 0.066f), std::bind(&NewGamePage::level_2_func, this));
  //level_3_button = new UiButton("ui_level3", vec2(0.0f, -0.15f), vec2(0.3f, 0.066f), std::bind(&NewGamePage::level_3_func, this));
}

//==============================================================================================
NewGamePage::~NewGamePage()
{
  delete level_1_button;
  delete level_2_button;
  //delete level_3_button;
  delete go_back_button;
}

//==============================================================================================
void NewGamePage::draw(UiDrawList& out) const
{
  level_1_button->draw(out);
  level_2_button->draw(out);
  //level_3_button->draw(out);
  go_back_button->draw(out);
}

//==============================================================================================
static void level_func(LevelName lvl)
{
  get_engine().send_event(ModuleEvent
    {
      .type = ModuleEventType::new_game_level_requested,
      .new_game = EventNewGame{.level = lvl},
    });
}

//==============================================================================================
void NewGamePage::level_1_func()
{
  level_func(Levels::LEVEL_1);
}

//==============================================================================================
void NewGamePage::level_2_func()
{
  level_func(Levels::LEVEL_2);
}

//==============================================================================================
void NewGamePage::level_3_func()
{
  level_func(Levels::LEVEL_3);
}

//============================================================================================
void OptionsPage::draw(UiDrawList& out) const
{
  sound_text->draw(out);
  music_text->draw(out);
  sensitivity_text->draw(out);
  crosshair_text->draw(out);
  shadow_text->draw(out);

  if (is_windowed)
  {
    windowed_button->draw(out);
  }
  else
  {
    fullscreen_button->draw(out);
  }

  if (is_shadows)
  {
    shadow_on_button->draw(out);
  }
  else
  {
    shadow_off_button->draw(out);
  }

  sound_volume_less->draw(out);
  sound_volume_more->draw(out);

  music_volume_less->draw(out);
  music_volume_more->draw(out);

  sensitivity_more->draw(out);
  sensitivity_less->draw(out);

  crosshair_less->draw(out);
  crosshair_more->draw(out);

  go_back_button->draw(out);

  // rendering of settings values
  vec2 positions[] = { vec2(0.4f, 0.30f), vec2(0.4f, 0.15f), vec2(0.4f, 0.0f) };
  int  steps[]     = { sound_step, music_step, sensitivity_step - 1 }; //sensitivity step is 1 - 10, but we can draw only 0 - 9

  const TextureManager& manager = TextureManager::get();
  const TextureHandle&  font    = manager["ui_font"];

  // draw numbers in the menu
  for (size_t i = 0; i < std::size(positions); i++)
  {
    out.add_char(font, positions[i], vec2(0.033f, 0.066f), steps[i] + '0');
  }

  // draw crosshair in menu, the texture is a grid of 11x2 crosshairs
  out.add_glyph(manager["ui_crosshair"], vec2(0.4f, -0.15f), vec2(0.04f, 0.066f), crosshair_step + 11, 11, 2);
}

//=============================================================================================
void OptionsPage::load_settings()
{
  std::ifstream f("settings.cfg");
  if (!f.is_open())
  {
    // we skip loading settings and use base values
    nc_warn("Failed to load settings");
    return;
  }
  auto data = nlohmann::json::parse(f);

  try
  {
    for (s32 sound_setting : data["sound"])
    {
      if (sound_setting < 0)
      {
        sound_setting = 0;
      }
      sound_step = min(9, sound_setting);
      get_engine().get_module<SoundSystem>().set_sound_volume(sound_step);
    }

    for (s32 music_setting : data["music"])
    {
      if (music_setting < 0)
      {
        music_setting = 0;
      }
      music_step = min(9, music_setting);
      get_engine().get_module<SoundSystem>().set_music_volume(music_step);
    }

    for (s32 sensitivity_setting : data["sensitivity"])
    {
      if (sensitivity_setting < 1)
      {
        sensitivity_setting = 1;
      }
      sensitivity_step = min(10, sensitivity_setting);
      get_engine().get_module<InputSystem>().set_sensitivity(sensitivity_step);
    }

    for (s32 crosshair_setting : data["crosshair"])
    {
      if (crosshair_setting < 0)
      {
        crosshair_setting = 0;
      }
      crosshair_step = min(9, crosshair_setting);
      get_engine().get_module<UserInterfaceSystem>().get_hud()->set_crosshair(crosshair_step);
    }

    for (bool fullscreen_setting : data["fullscreen"])
    {
      if (fullscreen_setting)
      {
        set_fullscreen();
      }
    }

    for (bool shadow_setting : data["shadows"])
    {
      if (shadow_setting)
      {
        shadow_on();
      }
      else
      {
        shadow_off();
      }
    }
  }
  catch (int) {}
}

//=============================================================================================
void OptionsPage::save_settings()
{
  std::ofstream f("settings.cfg");
  if (!f.is_open())
  {
    nc_warn("Failed to write settings");
  }
  nlohmann::json data;

  data["sound"] = sound_step;
  data["music"] = music_step;
  data["sensitivity"] = sensitivity_step;
  data["crosshair"] = crosshair_step;
  data["fullscreen"] = !is_windowed;
  data["shadows"] = is_shadows;

  f << data;
  f.close();
}

//=============================================================================================
void OptionsPage::set_windowed()
{
  SDL_Window* window = get_engine().get_module<GraphicsSystem>().get_window();
  SDL_SetWindowFullscreen(window, 0);
  is_windowed = true;
}

//=============================================================================================
void OptionsPage::set_fullscreen()
{
  SDL_Window* window = get_engine().get_module<GraphicsSystem>().get_window();
  SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
  is_windowed = false;
}

//=============================================================================================
void OptionsPage::set_sensitivity_less()
{
  sensitivity_step = max(1, sensitivity_step - 1);
  get_engine().get_module<InputSystem>().set_sensitivity(sensitivity_step);
}

//=============================================================================================
void OptionsPage::set_sensitivity_more()
{
  sensitivity_step = min(10, sensitivity_step + 1);
  get_engine().get_module<InputSystem>().set_sensitivity(sensitivity_step);
}

//=============================================================================================
void OptionsPage::set_sound_less()
{
  sound_step = max(0, sound_step - 1);
  get_engine().get_module<SoundSystem>().set_sound_volume(sound_step);
  SoundSystem::get().play_oneshot(Sounds::ui_click, 1.0f, SoundLayers::ui);
}

//=============================================================================================
void OptionsPage::set_sound_more()
{
  sound_step = min(9, sound_step + 1);
  get_engine().get_module<SoundSystem>().set_sound_volume(sound_step);
  SoundSystem::get().play_oneshot(Sounds::ui_click, 1.0f, SoundLayers::ui);
}

//=============================================================================================
void OptionsPage::set_music_less()
{
  music_step = max(0, music_step - 1);
  get_engine().get_module<SoundSystem>().set_music_volume(music_step);
}

//=============================================================================================
void OptionsPage::set_music_more()
{
  music_step = min(9, music_step + 1);
  get_engine().get_module<SoundSystem>().set_music_volume(music_step);
}

//=============================================================================================
void OptionsPage::set_crosshair_less()
{
  crosshair_step = (crosshair_step - 1);
  if (crosshair_step < 0)
  {
    crosshair_step = 9;
  }
  get_engine().get_module<UserInterfaceSystem>().get_hud()->set_crosshair(crosshair_step);
}

//=============================================================================================
void OptionsPage::set_crosshair_more()
{
  crosshair_step = (crosshair_step + 1) % 10;
  get_engine().get_module<UserInterfaceSystem>().get_hud()->set_crosshair(crosshair_step);
}

//=============================================================================================
void OptionsPage::shadow_on()
{
  is_shadows = true;
  get_engine().get_module<GraphicsSystem>().set_shadows(is_shadows);
}

//=============================================================================================
void OptionsPage::shadow_off()
{
  is_shadows = false;
  get_engine().get_module<GraphicsSystem>().set_shadows(is_shadows);
}

//=============================================================================================
OptionsPage::OptionsPage()
{
  sound_text = new UiButton("ui_sound", vec2(-0.3f, 0.30f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
  music_text = new UiButton("ui_music", vec2(-0.3f, 0.15f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
  sensitivity_text = new UiButton("ui_sensitivity", vec2(-0.3f, 0.0f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
  crosshair_text = new UiButton("ui_crosshair_text", vec2(-0.3f, -0.15f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
  shadow_text = new UiButton("ui_shadows", vec2(-0.3f, -0.3f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::do_nothing, this));
  shadow_on_button = new UiButton("ui_on", vec2(0.3f, -0.3f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::shadow_off, this));
  shadow_off_button = new UiButton("ui_off", vec2(0.3f, -0.3f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::shadow_on, this));
  fullscreen_button = new UiButton("ui_fullscreen", vec2(0.3f, -0.45f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::set_windowed, this));
  windowed_button = new UiButton("ui_windowed", vec2(0.3f, -0.45f), vec2(0.3f, 0.066f), std::bind(&OptionsPage::set_fullscreen, this));

  sound_volume_less = new UiButton("ui_arrow_left", vec2(0.3f, 0.30f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_sound_less, this));
  sound_volume_more = new UiButton("ui_arrow_right", vec2(0.5f, 0.30f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_sound_more, this));
  music_volume_less = new UiButton("ui_arrow_left", vec2(0.3f, 0.15f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_music_less, this));
  music_volume_more = new UiButton("ui_arrow_right", vec2(0.5f, 0.15f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_music_more, this));
  sensitivity_less = new UiButton("ui_arrow_left", vec2(0.3f, 0.0f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_sensitivity_less, this));
  sensitivity_more = new UiButton("ui_arrow_right", vec2(0.5f, 0.0f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_sensitivity_more, this));
  crosshair_less = new UiButton("ui_arrow_left", vec2(0.3f, -0.15f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_crosshair_less, this));
  crosshair_more = new UiButton("ui_arrow_right", vec2(0.5f, -0.15f), vec2(0.033f, 0.066f), std::bind(&OptionsPage::set_crosshair_more, this));

  go_back_button = new UiButton("ui_back", vec2(-0.3f, 0.45f), vec2(0.28f, 0.066f), std::bind(&OptionsPage::go_back, this));
}

//=============================================================================================
OptionsPage::~OptionsPage()
{
  delete sound_text;
  delete music_text;
  delete sensitivity_text;
  delete fullscreen_button;
  delete windowed_button;
  delete sound_volume_less;
  delete sound_volume_more;
  delete music_volume_less;
  delete music_volume_more;
  delete sensitivity_less;
  delete sensitivity_more;
  delete crosshair_less;
  delete crosshair_more;
  delete go_back_button;
}

//==============================================================================================
void OptionsPage::update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse)
{
  //update buttons and determine which (one) button can be pressed based on mouse position
  UiButton* hover_over_button = nullptr;

  fullscreen_button->set_hover(false);
  windowed_button->set_hover(false);

  sound_volume_less->set_hover(false);
  sound_volume_more->set_hover(false);

  music_volume_less->set_hover(false);
  music_volume_more->set_hover(false);

  sensitivity_less->set_hover(false);
  sensitivity_more->set_hover(false);

  crosshair_less->set_hover(false);
  crosshair_more->set_hover(false);

  go_back_button->set_hover(false);

  shadow_on_button->set_hover(false);
  shadow_off_button->set_hover(false);

  if (is_windowed)
  {
    if (windowed_button->is_point_in_rec(mouse_pos))
    {
      hover_over_button = windowed_button;
    }
  }
  else
  {
    if (fullscreen_button->is_point_in_rec(mouse_pos))
    {
      hover_over_button = fullscreen_button;
    }
  }

  if (is_shadows)
  {
    if (shadow_on_button->is_point_in_rec(mouse_pos))
    {
      hover_over_button = shadow_on_button;
    }
  }
  else
  {
    if (shadow_off_button->is_point_in_rec(mouse_pos))
    {
      hover_over_button = shadow_off_button;
    }
  }


  if (sound_volume_less->is_point_in_rec(mouse_pos))
  {
    hover_over_button = sound_volume_less;
  }
  else if (sound_volume_more->is_point_in_rec(mouse_pos))
  {
    hover_over_button = sound_volume_more;
  }
  else if (music_volume_less->is_point_in_rec(mouse_pos))
  {
    hover_over_button = music_volume_less;
  }
  else if (music_volume_more->is_point_in_rec(mouse_pos))
  {
    hover_over_button = music_volume_more;
  }
  else if (sensitivity_less->is_point_in_rec(mouse_pos))
  {
    hover_over_button = sensitivity_less;
  }
  else if (sensitivity_more->is_point_in_rec(mouse_pos))
  {
    hover_over_button = sensitivity_more;
  }
  else if (crosshair_less->is_point_in_rec(mouse_pos))
  {
    hover_over_button = crosshair_less;
  }
  else if (crosshair_more->is_point_in_rec(mouse_pos))
  {
    hover_over_button = crosshair_more;
  }
  else if (go_back_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = go_back_button;
  }

  if (hover_over_button != nullptr)
  {
    hover_over_button->set_hover(true);

    if (!(prev_mouse & SDL_BUTTON(1)) && (cur_mouse & SDL_BUTTON(1)))
    {
      hover_over_button->on_click();
    }
  }
}

//=============================================================================================
LoadGamePage::LoadGamePage()
{
  go_back_button = new UiButton("ui_back", vec2(0.0f, 0.6f), vec2(0.3f, 0.066f), std::bind(&LoadGamePage::go_back, this));
  page_up_button = new UiButton("ui_arrow_down", vec2(0.0f, -0.55f), vec2(0.045f, 0.03f), std::bind(&LoadGamePage::page_up, this));
  page_down_button = new UiButton("ui_arrow_up", vec2(0.0f, 0.45f), vec2(0.045f, 0.03f), std::bind(&LoadGamePage::page_down, this));
}

//=============================================================================================
LoadGamePage::~LoadGamePage()
{
  delete go_back_button;
  delete page_down_button;
  delete page_up_button;

  for (auto load_game_button : load_game_buttons)
  {
    delete load_game_button;
  }
  load_game_buttons.clear();
}

//=============================================================================================
void LoadGamePage::update_saves()
{
  //delete buttons
  for (auto load_game_button : load_game_buttons)
  {
    delete load_game_button;
  }
  load_game_buttons.clear();

  // starting pos for first button and steps for next
  vec2 pos      = vec2(-0.0f, 0.3f);
  vec2 step_pos = vec2(0.0f, -0.1f);
  vec2 scale    = vec2(0.5f, 0.033f);

  // create buttons
  auto saves = list_available_save_files();
  for (const std::string& save : saves)
  {
    load_game_buttons.push_back(new UiLoadGameButton(save, pos, scale));
    pos += step_pos;
    if (pos.y < -0.41f)
    {
      pos = vec2(-0.0f, 0.3f);
    }
  }

  page = 0;
}

//==============================================================================================
void LoadGamePage::update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse)
{
  //update buttons and determine which (one) button can be pressed based on mouse position
  go_back_button->set_hover(false);
  page_down_button->set_hover(false);
  page_up_button->set_hover(false);
  for (auto& button : load_game_buttons)
  {
    button->set_hover(false);
  }

  UiButton* hover_over_button = nullptr;

  if (go_back_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = go_back_button;
  }
  else if (page_down_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = page_down_button;
  }
  else if (page_up_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = page_up_button;
  }

  for (size_t i = 0 + page * PAGE_SIZE;
    i < load_game_buttons.size() && i < cast<size_t>((page + 1) * PAGE_SIZE);
    i++)
  {
    if (load_game_buttons[i]->is_point_in_rec(mouse_pos))
    {
      hover_over_button = load_game_buttons[i];
      break;
    }
  }

  if (hover_over_button != nullptr)
  {
    hover_over_button->set_hover(true);

    if (!(prev_mouse & SDL_BUTTON(1)) && (cur_mouse & SDL_BUTTON(1)))
    {
      hover_over_button->on_click();
    }
  }

}

//==============================================================================================
void LoadGamePage::draw(UiDrawList& out) const
{
  // render scrolling and go back buttons
  go_back_button->draw(out);
  page_down_button->draw(out);
  page_up_button->draw(out);

  //render load game buttons
  for (size_t i = 0 + page * PAGE_SIZE;
    i < load_game_buttons.size() && i < cast<size_t>((page + 1) * PAGE_SIZE);
    i++)
  {
    load_game_buttons[i]->draw(out);
  }
}

//=============================================================================================
void LoadGamePage::go_back()
{
  get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MenuPage::main_page);
}

//=============================================================================================
void LoadGamePage::page_up()
{
  page = min(page + 1, cast<s32>(load_game_buttons.size()) / PAGE_SIZE);
}

//=============================================================================================
void LoadGamePage::page_down()
{
  page = max(0, page - 1);
}

//=============================================================================================
void OptionsPage::go_back()
{
  get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MenuPage::main_page);
}

//=============================================================================================
void NewGamePage::go_back()
{
  get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MenuPage::main_page);
}

//==============================================================================================
QuitGamePage::QuitGamePage()
{
  yes_button = new UiButton("ui_yes", vec2(0.0f, 0.05f), vec2(0.3f, 0.066f), std::bind(&QuitGamePage::yes_func, this));
  no_button = new UiButton("ui_no", vec2(0.0f, -0.1f), vec2(0.3f, 0.066f), std::bind(&QuitGamePage::no_func, this));
}

//==============================================================================================
QuitGamePage::~QuitGamePage()
{
  delete yes_button;
  delete no_button;
}

//==============================================================================================
void QuitGamePage::update(vec2 mouse_pos, u32 prev_mouse, u32 cur_mouse)
{
  //update buttons and determine which (one) button can be pressed based on mouse position
  UiButton* hover_over_button = nullptr;

  yes_button->set_hover(false);
  no_button->set_hover(false);

  if (yes_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = yes_button;
  }
  else if (no_button->is_point_in_rec(mouse_pos))
  {
    hover_over_button = no_button;
  }

  if (hover_over_button != nullptr)
  {
    hover_over_button->set_hover(true);

    if (!(prev_mouse & SDL_BUTTON(1)) && (cur_mouse & SDL_BUTTON(1)))
    {
      hover_over_button->on_click();
    }
  }
}

//==============================================================================================
void QuitGamePage::draw(UiDrawList& out) const
{
  yes_button->draw(out);
  no_button->draw(out);
}

//==============================================================================================
void QuitGamePage::yes_func()
{
  get_engine().request_quit();
}

//==============================================================================================
void QuitGamePage::no_func()
{
  get_engine().get_module<UserInterfaceSystem>().get_menu_manager()->set_page(MenuPage::main_page);
}

//===========================================================================================
MainMenuPage::MainMenuPage()
{
  const char* ng_text = "ui_new_game";
  const char* o_text = "ui_options";
  const char* lg_text = "ui_load_game";
  const char* sg_text = "ui_save_game";
  const char* q_text = "ui_quit";

  new_game_button = new UiButton(ng_text, vec2(0.0f, 0.15f), vec2(0.3f, 0.066f), std::bind(&MainMenuPage::new_game_func, this));
  options_button = new UiButton(o_text, vec2(0.0f, 0.0f), vec2(0.3f, 0.066f), std::bind(&MainMenuPage::options_func, this));
  save_button = new UiButton(sg_text, vec2(0.0f, -0.15f), vec2(0.3f, 0.066f), std::bind(&MainMenuPage::save_game_func, this));
  load_button = new UiButton(lg_text, vec2(0.0f, -0.3f), vec2(0.3f, 0.066f), std::bind(&MainMenuPage::load_game_func, this));
  quit_button = new UiButton(q_text, vec2(0.0f, -0.45f), vec2(0.3f, 0.066f), std::bind(&MainMenuPage::quit_func, this));
  nuclidean_text = new UiButton("ui_nuclidean", vec2(0.0f, 0.5f), vec2(0.42f, 0.1f), std::bind(&MainMenuPage::quit_func, this));
}

//============================================================================================
MainMenuPage::~MainMenuPage()
{
  delete new_game_button;
  delete options_button;
  delete load_button;
  delete save_button;
  delete quit_button;
  delete nuclidean_text;
}
}
