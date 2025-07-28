#include <engine/ui/user_interface_module.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/engine.h>

#include <engine/player/player.h>
#include <engine/player/thing_system.h>

namespace nc
{
  EngineModuleId nc::UserInterfaceSystem::get_module_id()
  {
    return EngineModule::user_interface_system;
  }

  UserInterfaceSystem& UserInterfaceSystem::get()
  {
    return get_engine().get_module<UserInterfaceSystem>();
  }

  bool UserInterfaceSystem::init()
  {
    return true;
  }

  void UserInterfaceSystem::gather_player_info()
  {
    display_health = get_engine().get_module<ThingSystem>().get_player()->get_health();
  }

  void UserInterfaceSystem::draw()
  {
  }

  void UserInterfaceSystem::on_event(ModuleEvent& event)
  {
    switch (event.type)
    {
    case ModuleEventType::post_init:
      break;
    case ModuleEventType::game_update:
      gather_player_info();
      break;
    case ModuleEventType::render:
      draw();
      break;
    case ModuleEventType::cleanup:
      break;
    default:
      break;
    }
  }

}


