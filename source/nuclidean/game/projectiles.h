// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <game/game_types.h>
#include <math/vector.h>

#include <engine/database/database.h>

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
  rail,

  // enemy ones
  fire_ball,
  possessed_fist,
  grunt_shot,

  // misc..

  //-//
  count
};

}

struct ProjectileStats
{
  f32  dmg_falloff    = 0; // per meter travelled
  f32  speed          = 0; // m/s
  f32  radius         = 0; // m
  f32  gravity        = 0; // how much to fall
  f32  friction       = 1; // slowdown per collision, [0-1] range, 1 = none
  f32  lifetime       = 0; // how long alive in seconds, 0 for inf
  s32  damage         = 0; // units
  s32  bounce_cnt     = 0; // if should bounce and how many times, 0 = death on hit
  cstr sprite         = nullptr; // Null for invisible
  u8   sprite_cnt     = 0;
  f32  anim_len       = 0.0f;
  vec3 light_color    = VEC3_ZERO;
  f32  light_cycle_len = 0.0f;
  cstr light_string   = nullptr;
  cstr hit_sprite     = nullptr;
  u8   hit_sprite_cnt = 0;
  f32  hit_sprite_len = 0.0f;
  vec3 hit_sprite_col = VEC3_ZERO;
  f32  hit_sprite_rad = 0.0f;
  u8   penetration_cnt = 0;
};

struct ProjectileStatsDb
{
  DbCol<f32  , "dmg falloff"      > dmg_falloff     = 0;
  DbCol<f32  , "speed"            > speed           = 0;
  DbCol<f32  , "radius"           > radius          = 0;
  DbCol<f32  , "gravity"          > gravity         = 0;
  DbCol<f32  , "friction"         > friction        = 1;
  DbCol<f32  , "lifetime"         > lifetime        = 0;
  DbCol<s32  , "damage"           > damage          = 0;
  DbCol<s32  , "bounce count"     > bounce_cnt      = 0;
  DbCol<Token, "sprite"           > sprite          = nullptr;
  DbCol<u8   , "sprite count"     > sprite_cnt      = 0;
  DbCol<f32  , "anim length"      > anim_len        = 0.0f;
  DbCol<vec3 , "light color"      > light_color     = VEC3_ZERO;
  DbCol<f32  , "light len cycle"  > light_cycle_len = 0.0f;
  DbCol<Token, "light string"     > light_string    = nullptr;
  DbCol<Token, "hit sprite"       > hit_sprite      = nullptr;
  DbCol<u8   , "hit sprite count" > hit_sprite_cnt  = 0;
  DbCol<f32  , "hit sprite length"> hit_sprite_len  = 0.0f;
  DbCol<vec3 , "hit sprite color" > hit_sprite_col  = VEC3_ZERO;
  DbCol<f32  , "hit sprite radius"> hit_sprite_rad  = 0.0f;
  DbCol<u8   , "penetration count"> penetration_cnt = 0;
};

extern ProjectileStats PROJECTILE_STATS[];

}
