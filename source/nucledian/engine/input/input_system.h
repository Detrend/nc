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

class InputSystem : public IEngineModule
{
public:
  static EngineModuleId get_module_id();
  static InputSystem& get();
  void on_event(ModuleEvent& event) override;
  bool init();

  void update_window_and_pump_messages();

  // Backdoor for replay system. This enables us to override player
  // inputs.
  void override_player_inputs(const PlayerSpecificInputs& player_inputs);

  void handle_app_event(const SDL_Event& event);
  void get_player_inputs();
  GameInputs get_inputs() const;
  GameInputs get_prev_inputs() const;

  void set_sensitivity(int step);

private:
  GameInputs m_current_inputs;
  GameInputs m_previous_inputs;

  float SENSITIVITY = -0.00125f;

  bool m_disable_player_inputs : 1 = false;

  
};

}

