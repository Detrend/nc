// Project Nucledian Source File
#include <game/enemies.h>
#include <game/projectiles.h>

namespace nc
{

EnemyStats ENEMY_STATS[] =
{
  // Cultist
  EnemyStats
  {
    .move_speed = 1.0f, .projectile = ProjectileTypes::fire_ball,
    .max_hp = 100,
  },

  // Possessed
  EnemyStats{},
};
static_assert(ARRAY_LENGTH(ENEMY_STATS) == EnemyTypes::count);

}
