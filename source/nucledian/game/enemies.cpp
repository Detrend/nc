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
    .move_speed = 1.0f,
    .projectile = ProjectileTypes::fire_ball,
    .max_hp = 100,
    .state_sprite_cnt
    {
      1,  // idle
      16, // walk
      45, // attack
      1,  // dead
      20, // dying
    },
    .state_sprite_len
    {
      1.0f,
      1.25f,
      3.75f,
      2.0f,
      1.6f,
    },
    .attack_frame = 26,
  },

  // Possessed
  EnemyStats
  {
    .move_speed = 3.0f,
    .projectile = ProjectileTypes::fire_ball,
    .max_hp = 150,
    .atk_delay_min = 0.25f,
    .atk_delay_max = 0.5f,
    .state_sprite_cnt
    {
      1,  // idle
      16, // walk
      23, // attack
      1,  // dead
      20, // dying
    },
    .state_sprite_len
    {
      1.0f,
      1.3f,
      1.91f,
      2.0f,
      1.6f,
    },
    .attack_frame = 7,
    .is_melee = true,
  },
};
static_assert(ARRAY_LENGTH(ENEMY_STATS) == EnemyTypes::count);

}
