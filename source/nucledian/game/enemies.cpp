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
    .move_speed = 2.5f,
    .projectile = ProjectileTypes::fire_ball,
    .max_hp     = 100,
    .height     = 2.0f,
    .eye_height = 1.85f,
    .atk_height = 1.6f,
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
      2.0f,
      2.0f,
      1.6f,
    },
    .attack_frame = 26,
    .infight_chance = 0.05f // 5%
  },

  // Possessed
  EnemyStats
  {
    .move_speed    = 5.0f,
    .projectile    = ProjectileTypes::fire_ball,
    .max_hp        = 200,
    .atk_delay_min = 0.25f,
    .atk_delay_max = 0.5f,
    .state_sprite_cnt
    {
      1,  // idle
      16, // walk
      23, // attack
      1,  // dead
      24, // dying
    },
    .state_sprite_len
    {
      1.0f,
      1.0f,
      1.0f,
      2.0f,
      1.6f,
    },
    .attack_frame = 7,
    .is_melee = true,
    .infight_chance = 0.0f,
  },
};
static_assert(ARRAY_LENGTH(ENEMY_STATS) == EnemyTypes::count);

}
