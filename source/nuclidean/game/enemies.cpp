// Project Nuclidean Source File
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
    .max_hp     = 80,
    .height     = 1.9f,
    .eye_height = 1.75f,
    .atk_height = 1.5f,
    .radius     = 0.35f,
    .atk_delay_min = 0.5f,
    .atk_delay_max = 2.5f,
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
      1.5f,
      2.0f,
      1.6f,
    },
    .attack_frame = 26,
    .infight_chance = 0.05f, // 5%
    .step_height = 0.9f,
  },

  // Possessed
  EnemyStats
  {
    .move_speed    = 4.0f,
    .projectile    = ProjectileTypes::possessed_fist,
    .max_hp        = 200,
    .height        = 1.9f,
    .eye_height    = 1.75f,
    .atk_height    = 1.5f,
    .radius        = 0.4f,
    .atk_delay_min = 0.2f,
    .atk_delay_max = 0.4f,
    .state_sprite_cnt
    {
      1,  // idle
      16, // walk
      23, // attack
      1,  // dead
      27, // dying
    },
    .state_sprite_len
    {
      1.0f,
      1.0f,
      1.5f,
      2.0f,
      1.6f,
    },
    .attack_frame   = 7,
    .is_melee       = true,
    .infight_chance = 0.0f,
    .step_height    = 1.3f,
  },

  // Grunt
  EnemyStats
  {
    .move_speed = 2.0f,
    .projectile = ProjectileTypes::grunt_shot,
    .max_hp = 100,
    .height = 1.5f,
    .eye_height = 1.4f,
    .atk_height = 0.8f,
    .radius = 0.4f,
    .atk_delay_min = 0.2f,
    .atk_delay_max = 0.4f,
    .state_sprite_cnt
    {
      1,  // idle
      16, // walk
      30, // attack
      1,  // dead
      28, // dying
    },
    .state_sprite_len
    {
      1.0f,
      1.0f,
      1.5f,
      2.0f,
      2.0f,
    },
    .attack_frame = 17,
    .is_melee = false,
    .infight_chance = 0.2f, //20%
    .step_height    = 0.9f,
  },
};
static_assert(ARRAY_LENGTH(ENEMY_STATS) == EnemyTypes::count);

}
