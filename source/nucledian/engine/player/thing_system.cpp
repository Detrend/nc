// Project Nucledian Source File
#include <engine/player/thing_system.h>
#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>
#include <engine/input/input_system.h>
#include <intersect.h>

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
      GameInputs curInputs = get_engine().get_module<InputSystem>().get_inputs();
      GameInputs prevInputs = get_engine().get_module<InputSystem>().get_prev_inputs();

      player.get_wish_velocity(curInputs, event.update.dt);
      for (size_t i = 0; i < enemies.size(); i++)
      {
        enemies[i].get_wish_velocity(event.update.dt);
      }

      bool didAttack = player.get_attack_state(curInputs, prevInputs, event.update.dt);
      if (didAttack)
      {
        [[maybe_unused]] vec3 rayStart = player.get_position();
        [[maybe_unused]] vec3 rayEnd = rayStart + player.get_look_direction() * 50.0f;

        f32 hitDistance = 999999;

        for (size_t i = 0; i < enemies.size(); i++)
        {
          const f32   width = enemies[i].get_width();
          const f32   height = enemies[i].get_height() * 2.0f;
          const vec3  position = enemies[i].get_position();

          const aabb3 bbox = aabb3
          {
            position - vec3{width, 0.0f,   width},
            position + vec3{width, height, width}
          };

          f32 out;
          vec3 normal;
          if (intersect::ray_aabb3(rayStart, rayEnd, bbox, out, normal))
          {
            hitDistance = out;
          }
        }
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

