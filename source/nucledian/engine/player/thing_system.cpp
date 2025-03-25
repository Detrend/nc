#include <engine/player/thing_system.h>
#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>
#include <engine/input/input_system.h>
#include <iostream>

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
    case ModuleEventType::post_init:
    {
      enemies.push_back(Enemy(vec3(0, 0, 0)));
      break;
    }
    case ModuleEventType::game_update:
    {
      player.update(get_engine().get_module<InputSystem>().get_inputs());
      for (size_t i = 0; i < enemies.size(); i++)
      {
        enemies[i].update();
      }

      if (player.did_collide(enemies[0]))
      {
        std::cout << "Collision" << std::endl;
      }
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

