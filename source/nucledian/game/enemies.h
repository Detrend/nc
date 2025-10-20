// Project Nucledian Source File
#pragma once

#include <types.h>
#include <game/game_types.h>

#include <metaprogramming.h> // ARRAY_LENGTH

namespace nc
{

namespace EnemyTypes
{

enum evalue : EnemyType
{
  cultist,
  possessed,

  // - //
  count
};

}

constexpr cstr ENEMY_TYPE_NAMES[]
{
  "cultist",
  "possessed",
};
static_assert(ARRAY_LENGTH(ENEMY_TYPE_NAMES) == EnemyTypes::count);

struct EnemyStats
{
  f32            move_speed    = 5.0f;
  ProjectileType projectile    = 0;
  s32            max_hp        = 100;
  f32            height        = 2.5f;
  f32            radius        = 1.0f;
  f32            atk_delay_min = 3.0f;
  f32            atk_delay_max = 8.0f;
  bool           is_melee : 1  = false;
};

extern EnemyStats ENEMY_STATS[];

}
