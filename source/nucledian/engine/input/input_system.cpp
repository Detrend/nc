// Project Nucledian Source File
#include <engine/input/input_system.h>

#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>
#include <engine/core/engine.h>

#include <imgui/imgui_impl_sdl2.h>

#include <SDL2/include/SDL.h>

#include <utility> // std::pair

namespace nc
{

//==============================================================================
void handle_player_input(GameInputs& inputs)
{
  namespace Keys      = PlayerKeyInputs;
  namespace Analogues = PlayerAnalogInputs;

  const u8* keyboard_state = SDL_GetKeyboardState(nullptr);
  nc_assert(keyboard_state);

  constexpr auto KEY_MAPPINGS = std::array
  {
    std::pair{SDL_SCANCODE_W,     Keys::forward},
    std::pair{SDL_SCANCODE_S,     Keys::backward},
    std::pair{SDL_SCANCODE_A,     Keys::left},
    std::pair{SDL_SCANCODE_D,     Keys::right},
    std::pair{SDL_SCANCODE_SPACE, Keys::jump},
    std::pair{SDL_SCANCODE_E,     Keys::use},
  };

  // Movement
  for (const auto[scancode, key_idx] : KEY_MAPPINGS)
  {
    if (keyboard_state[scancode])
    {
      inputs.player_inputs.keys |= 1 << key_idx;
    }
  }

  // Weapons
  for (int i = 0; i < Keys::last_weapon - Keys::first_weapon + 1; ++i)
  {
    if (keyboard_state[SDL_SCANCODE_1 + i])
    {
      inputs.player_inputs.keys |= 1 << (Keys::weapon_0 + i);
    }
  }

  // Look around
  int x, y;
  u32 mouse_code = SDL_GetRelativeMouseState(&x, &y);

  f32 sensitivity = get_engine().get_module<InputSystem>().get_sensitivity();
  inputs.player_inputs.analog[Analogues::look_vertical  ] = y * sensitivity;
  inputs.player_inputs.analog[Analogues::look_horizontal] = x * sensitivity;

  // Shooting
  if (SDL_BUTTON(mouse_code) == 1)
  {
    inputs.player_inputs.keys |= 1 << Keys::primary;
  }
}
  
//==============================================================================
EngineModuleId InputSystem::get_module_id()
{
  return EngineModule::input_system;
}

//==============================================================================
InputSystem& InputSystem::get()
{
  return get_engine().get_module<InputSystem>();
}

//==============================================================================
void InputSystem::on_event(ModuleEvent& event)
{
  switch (event.type)
  {
    case ModuleEventType::cleanup:
    {
      // store the old inputs and reset the current ones
      m_previous_inputs = m_current_inputs;
      m_current_inputs  = GameInputs{};
    }
    break;
  }
}

//==============================================================================
bool InputSystem::init()
{
  return true;
}

//==============================================================================
void InputSystem::update_window_and_pump_messages()
{
  SDL_Event event;

  while (SDL_PollEvent(&event))
  {
#ifdef NC_IMGUI
    // this needs to be done so that ImGui can handle keyboard input or clicking
    // on buttons/checkboxes
    ImGui_ImplSDL2_ProcessEvent(&event);
#endif

    // distribute the message to the engine
    get_engine().process_window_event(event);

    // consume the message ourselves
    this->handle_app_event(event);
  }

  handle_player_input(m_current_inputs);
}

//==============================================================================
void InputSystem::handle_app_event([[maybe_unused]]const SDL_Event& event)
{
  
}

//==============================================================================
GameInputs InputSystem::get_inputs() const
{
  // MR says: workaround for now
  if (this->are_game_inputs_disabled())
  {
    return GameInputs{};
  }

  return m_current_inputs;
}

//==============================================================================
GameInputs InputSystem::get_prev_inputs() const
{
  if (this->are_game_inputs_disabled())
  {
    return GameInputs{};
  }

  return m_previous_inputs;
}

void InputSystem::set_sensitivity(int step)
{
  float ZERO_SENSITIVITY = -0.0003125f;

  SENSITIVITY = ZERO_SENSITIVITY * step;
}

float InputSystem::get_sensitivity()
{
  return SENSITIVITY;
}

//==============================================================================
void InputSystem::lock_player_input(InputLockLayer layer, bool lock)
{
  bool disabled_previously = !!m_locked_inputs;

  if (lock)
  {
    m_locked_inputs |= layer;
  }
  else
  {
    m_locked_inputs &= ~layer;
  }

  bool disabled_now = !!m_locked_inputs;

  if (disabled_previously != disabled_now)
  {
    // Do nothing
    return;
  }

  // Switch
  SDL_SetRelativeMouseMode(disabled_now ? SDL_FALSE : SDL_TRUE);
}

//==============================================================================
bool InputSystem::are_game_inputs_disabled() const
{
  return !!m_locked_inputs;
}

}
