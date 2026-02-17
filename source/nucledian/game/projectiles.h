// Project Nucledian Source File
#pragma once

#include <types.h>
#include <game/game_types.h>
#include <math/vector.h>

namespace nc
{

namespace ProjectileTypes
{

enum evalue : ProjectileType
{
  // player ones
  wrench,
  plasma_ball,
  shotgun_slug,

  // enemy ones
  fire_ball,
  possessed_fist,

  // misc..

  //-//
  count
};

}

struct ProjectileStats
{
  f32  dmg_falloff = 0; // per meter travelled
  f32  speed       = 0; // m/s
  f32  radius      = 0; // m
  f32  gravity     = 0; // how much to fall
  f32  friction    = 1; // slowdown per collision, [0-1] range, 1 = none
  f32  lifetime    = 0; // how long alive in seconds, 0 for inf
  s32  damage      = 0; // units
  s32  bounce_cnt  = 0; // if should bounce and how many times, 0 = death on hit
  cstr sprite      = nullptr; // Null for invisible
  u8   sprite_cnt  = 0;
  f32  anim_len    = 0.0f;
  vec3 light_color = VEC3_ZERO;
};

extern ProjectileStats PROJECTILE_STATS[];

}
