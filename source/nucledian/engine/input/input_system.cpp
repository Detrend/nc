// Project Nucledian Source File
#include <engine/input/input_system.h>

#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>
#include <engine/core/engine.h>

#include <SDL2/include/SDL.h>

namespace nc
{
  
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
  case ModuleEventType::game_update:
  {
    get_player_inputs();
    debug_player.update(m_current_inputs);
    break;
  }
    case ModuleEventType::cleanup:
    {
      // store the old inputs and reset the current ones
      m_previous_inputs = m_current_inputs;
      m_current_inputs = GameInputs{};
      break;
    }
  }
}

//==============================================================================
bool InputSystem::init()
{
  debug_player = Player();
  return true;
}

//==============================================================================
void InputSystem::override_player_inputs(const PlayerSpecificInputs& new_inputs)
{
  m_current_inputs.player_inputs = new_inputs;
}

//==============================================================================
void InputSystem::handle_app_event([[maybe_unused]]const SDL_Event& event)
{
  // expected to do this after COPYing previous input to current

  f32 xrel = 0;
  f32 yrel = 0;

  switch (event.type) 
  {
  case SDL_MOUSEMOTION:
    xrel = event.motion.xrel * 0.010f; // needs to be replaced by sensitivity
    yrel = event.motion.yrel * 0.010f;
    m_current_inputs.player_inputs.analog[PlayerAnalogInputs::look_horizontal] = -xrel;
    m_current_inputs.player_inputs.analog[PlayerAnalogInputs::look_vertical] = -yrel;
    break;
  case SDL_KEYDOWN:
    switch (event.key.keysym.scancode) 
    {
    case SDL_SCANCODE_W:
      m_current_inputs.player_inputs.keys = m_current_inputs.player_inputs.keys & (1 << PlayerKeyInputs::forward);
      break;
    case SDL_SCANCODE_S:
      m_current_inputs.player_inputs.keys = m_current_inputs.player_inputs.keys & (1 << PlayerKeyInputs::backward);
      break;
    case SDL_SCANCODE_A:
      m_current_inputs.player_inputs.keys = m_current_inputs.player_inputs.keys & (1 << PlayerKeyInputs::left);
      break;
    case SDL_SCANCODE_D:
      m_current_inputs.player_inputs.keys = m_current_inputs.player_inputs.keys & (1 << PlayerKeyInputs::right);
      break;
    default:
      break;
    }
    break;
  case SDL_KEYUP:
    switch (event.key.keysym.scancode)
    {
    case SDL_SCANCODE_W:
      m_current_inputs.player_inputs.keys = m_current_inputs.player_inputs.keys ^ (1 << PlayerKeyInputs::forward);
      break;
    case SDL_SCANCODE_S:
      m_current_inputs.player_inputs.keys = m_current_inputs.player_inputs.keys ^ (1 << PlayerKeyInputs::backward);
      break;
    case SDL_SCANCODE_A:
      m_current_inputs.player_inputs.keys = m_current_inputs.player_inputs.keys ^ (1 << PlayerKeyInputs::left);
      break;
    case SDL_SCANCODE_D:
      m_current_inputs.player_inputs.keys = m_current_inputs.player_inputs.keys ^ (1 << PlayerKeyInputs::right);
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
}

//==============================================================================

void InputSystem::get_player_inputs()
{
  SDL_SetRelativeMouseMode(SDL_TRUE);
  const u8* keyboard_state = SDL_GetKeyboardState(nullptr);

  if (keyboard_state[SDL_SCANCODE_W])
    m_current_inputs.player_inputs.keys = m_current_inputs.player_inputs.keys | (1 << PlayerKeyInputs::forward);
  if (keyboard_state[SDL_SCANCODE_S])
    m_current_inputs.player_inputs.keys = m_current_inputs.player_inputs.keys | (1 << PlayerKeyInputs::backward);
  if (keyboard_state[SDL_SCANCODE_A])
    m_current_inputs.player_inputs.keys = m_current_inputs.player_inputs.keys | (1 << PlayerKeyInputs::left);
  if (keyboard_state[SDL_SCANCODE_D])
    m_current_inputs.player_inputs.keys = m_current_inputs.player_inputs.keys | (1 << PlayerKeyInputs::right);

  int x, y;
  SDL_GetRelativeMouseState(&x, &y);

  f32 sensitivity = 0.05f;
  m_current_inputs.player_inputs.analog[PlayerAnalogInputs::look_vertical] -= y * sensitivity;
  m_current_inputs.player_inputs.analog[PlayerAnalogInputs::look_horizontal] += x * sensitivity;
}

//==============================================================================
const GameInputs& InputSystem::get_inputs() const
{
  return m_current_inputs;
}

Player* InputSystem::get_player()
{
  return &debug_player;
}

}

