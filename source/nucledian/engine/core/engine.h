// Project Nucledian Source File
#pragma once

// MR says: Try to not include anything here as the engine is included by
// pretty much everything and therefore would make the include chain too deep
// and cause longer builds.
// Instead forward declare everything and do the includes in the .cpp file.

#include <engine/core/is_engine_module.h>

#include <array>
#include <memory>
#include <vector>
#include <string>

union SDL_Event;

namespace nc
{

struct ModuleEvent;
class  IEngineModule;
struct MapSectors;

// Forward declare token and level names so that we do not have to include them
template<size_t TTokenCount>
struct CompositeToken;
using Token = CompositeToken<2>;
using LevelName = Token;

using CmdArgs = std::vector<std::string>;
class Engine
{
public:
  template<typename Module>
    requires IsEngineModule<Module>
  Module& get_module();

  void send_event(ModuleEvent& event);
  void send_event(ModuleEvent&& event);

  bool init(const CmdArgs& cmd_args);
  void run();
  void terminate();

  const MapSectors& get_map();

  // TODO: if we use this in multiple places then
  // probably introduce a quit reason enum as well
  void request_quit();

  void process_window_event(const SDL_Event& evnt);

  f32  get_delta_time() const;

  u64  get_frame_idx() const;

  bool is_game_paused() const;

  // Called first before level end if a demo was playing
  // menu:     schedule next demo
  // gameplay: do nothing, gets handled by itself
  // console:  end the game
  void on_demo_end();

  void on_level_end();

  bool should_ammo_hp_hud_be_visible() const;

  // Framerates of the recorded demo and the running system can be different.
  // If true then the game system tries to stabilize this by simulating more or
  // less demo frames per one game frame. Should be always true except when you
  // want to simulate exactly one demo frame per one game frame.
  bool should_run_demo_proportional_speed() const;

  // If true then the menu should be permanently visible and can't be hidded
  // with pressing of a keybind
  bool is_menu_locked_visible() const;

private:
  // Called from the UI if the menu opens/closes
  void on_menu_state_changed(bool opened);

  void pause(bool pause);

  // Has to change the state
  void on_new_game_selected_from_menu(LevelName level);

  bool should_quit() const;
  void play_random_demo();

  void on_event(const ModuleEvent& event);

  // Either boots into the menu after the start (default) or plays a demo.
  bool handle_post_init_game_startup(const CmdArgs& args);

private:
  using ModuleArray  = std::array<std::unique_ptr<IEngineModule>, 8>;
  using ModuleVector = std::vector<IEngineModule*>;

  enum class GameState : u8
  {
    in_menu,            // in menu, the demo is playing in the background
    player_handled,     // the player is playing
    debug_playing_demo, // running the demo from console
  };

private:
  ModuleArray   m_modules;
  ModuleVector  m_module_init_order;
  f32           m_delta_time = 0.0f; // last frame time in seconds
  u64           m_frame_idx = 0;     // index of a frame, currently only for debug
  GameState     m_game_state = GameState::in_menu;
  bool          m_should_quit       : 1 = false;
  bool          m_demo_adjust_speed : 1 = true;
  bool          m_paused            : 1 = false;
};

Engine& get_engine();

int init_engine_and_run_game(const CmdArgs& args);

}

#include <engine/core/engine.inl>
