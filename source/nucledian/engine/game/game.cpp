// Project Nucledian Source File

#include <engine/game/game.h>

// Entity types
#include <engine/player/player.h>
#include <engine/enemies/enemy.h>
#include <game/projectile.h>

// Other
#include <engine/map/map_system.h>
#include <engine/map/map_dynamics.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/sector_mapping.h>
#include <game/entity_attachment_manager.h>

#include <engine/input/game_input.h>

namespace nc
{

//==============================================================================
void Game::update
(
  f32                  dt,
  PlayerSpecificInputs curr_input,
  PlayerSpecificInputs prev_input
)
{
  // Handle the player first
  entities->for_each<Player>([&](Player& player)
  {
    player.update(curr_input, prev_input, dt);
  });

  // Handle enemies
  entities->for_each<Enemy>([&](Enemy& enemy)
  {
    enemy.update(dt);
  });

  // Handle projectiles
  entities->for_each<Projectile>([&](Projectile& proj)
  {
    proj.update(dt);
  });

  // And update the map
  dynamics->update(dt);

  // Push the frame index
  frame_idx += 1;

  // And then clean up the dead entities
  entities->cleanup();
}

}
