// Project Nucledian Source File
#include <engine/input/input_system.h>

#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>
#include <engine/core/engine.h>

#include <imgui/imgui_impl_sdl2.h>

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
  // MR says: remove this later, keep it just for now.
  SDL_SetRelativeMouseMode(m_disable_player_inputs ? SDL_FALSE : SDL_TRUE);
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
}

//==============================================================================
void InputSystem::override_player_inputs(const PlayerSpecificInputs& new_inputs)
{
  m_current_inputs.player_inputs = new_inputs;
}

//==============================================================================
void InputSystem::handle_app_event(const SDL_Event& event)
{
  switch (event.type) 
  {
  case SDL_KEYDOWN:
    switch (event.key.keysym.scancode) 
    {
    // MR says: workaround for showing up cursor
    case SDL_SCANCODE_ESCAPE:
    {
      m_disable_player_inputs = !m_disable_player_inputs;
      SDL_SetRelativeMouseMode(m_disable_player_inputs ? SDL_FALSE : SDL_TRUE);
      break;
    }
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
  u32 mouseCode = SDL_GetRelativeMouseState(&x, &y);

  f32 sensitivity = -1.0f / 800.0f;
  m_current_inputs.player_inputs.analog[PlayerAnalogInputs::look_vertical]   = y * sensitivity;
  m_current_inputs.player_inputs.analog[PlayerAnalogInputs::look_horizontal] = x * sensitivity;

  if (SDL_BUTTON(mouseCode) == 1)
  {
    m_current_inputs.player_inputs.keys = m_current_inputs.player_inputs.keys | (1 << PlayerKeyInputs::primary);
  }
}

//==============================================================================
GameInputs InputSystem::get_inputs() const
{
  // MR says: workaround for now
  if (m_disable_player_inputs)
  {
    return GameInputs{};
  }

  return m_current_inputs;
}

}

