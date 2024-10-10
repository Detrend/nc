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
  // TODO
}

//==============================================================================
const GameInputs& InputSystem::get_inputs() const
{
  return m_current_inputs;
}

}

