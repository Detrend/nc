#include <engine/player/thing_system.h>
#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>
#include <engine/input/input_system.h>

namespace nc
{
  EngineModuleId nc::ThingSystem::get_module_id()
  {
    return EngineModule::entity_system;
  }

  //==========================================================

  ThingSystem& ThingSystem::get()
  {
    return get_engine().get_module<ThingSystem>();
  }

  //==========================================================

  bool ThingSystem::init()
  {
    player = Player(vec3(0, 0, 3.0f));
    return true;
  }

  //==========================================================

  void ThingSystem::on_event(ModuleEvent& event)
  {
    switch (event.type) {
    case ModuleEventType::game_update:
    {
      player.update(get_engine().get_module<InputSystem>().get_inputs());
      break;
    }
    default:
      break;
    }
  }

  //==========================================================

  Player* ThingSystem::get_player()
  {
    return &player;
  }
}

