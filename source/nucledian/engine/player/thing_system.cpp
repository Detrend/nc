// Project Nucledian Source File
#include <engine/player/thing_system.h>
#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>
#include <engine/input/input_system.h>

namespace nc
{

//==========================================================
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
  switch (event.type)
  {
    case ModuleEventType::post_init:
    {
      enemies.push_back(Enemy(vec3(0, 0, 0), vec3(0, 0, -1)));
      break;
    }

    case ModuleEventType::game_update:
    {
      //INPUT PHASE
      player.get_wish_velocity(get_engine().get_module<InputSystem>().get_inputs(), event.update.dt);
      for (size_t i = 0; i < enemies.size(); i++)
      {
        enemies[i].get_wish_velocity(event.update.dt);
      }

      //CHECK FOR COLLISIONS
      for (size_t i = 0; i < enemies.size(); i++)
      {
        player.check_collision(enemies[i]);
      }

      for (size_t i = 0; i < enemies.size(); i++)
      {
        enemies[i].check_for_collision(player);
      }

      //FINAL VELOCITY CHANGE
      player.apply_velocity();

      for (size_t i = 0; i < enemies.size(); i++)
      {
        enemies[i].apply_velocity();
      }

      break;
    }
  }
}

//==========================================================
Player* ThingSystem::get_player()
{
  return &player;
}

//==========================================================
const std::vector<Enemy>& ThingSystem::get_enemies() const
{
  return enemies;
}

}

