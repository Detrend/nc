// Project Nucledian Source File
#pragma once

#include <engine/core/engine_module_id.h>
#include <engine/core/engine_module.h>

#include <engine/input/game_input.h>

#include <types.h>

#include <engine/player/player.h>

union SDL_Event;

namespace nc
{

struct ModuleEvent;

using InputLockLayer = u8;
namespace InputLockLayers
{
enum evalue : InputLockLayer
{
  menu  = 1 << 0, // Locked by the menu
  cvars = 1 << 1, // Locked by the cvars
};
};

class InputSystem : public IEngineModule
{
public:
  static EngineModuleId get_module_id();
  static InputSystem& get();
  void on_event(ModuleEvent& event) override;
  bool init();

  void update_window_and_pump_messages();

  void handle_app_event(const SDL_Event& event);
  void get_player_inputs();
  GameInputs get_inputs() const;
  GameInputs get_prev_inputs() const;

  void  set_sensitivity(int step);
  float get_sensitivity();

  // Disables mouse and movement. Can be used by the menu or some screen that
  // requires mouse or keyboard inputs.
  void lock_player_input(InputLockLayer layer, bool lock);

  bool are_game_inputs_disabled() const;

private:
  GameInputs m_current_inputs;
  GameInputs m_previous_inputs;

  float SENSITIVITY = -0.00125f;

  InputLockLayer m_locked_inputs = 0;

  
};

}

