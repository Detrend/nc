// Project Nucledian Source File
#pragma once

#include <types.h>

#include <game/game_types.h>
#include <engine/sound/sound_types.h> // SoundID

// Include this file inside the .cpps, but not in the headers. We do not want
// to rebuild the entire solution each time someone adds new item to enum or
// tweaks something.

namespace nc
{

namespace WeaponTypes
{

enum evalue : WeaponType
{
  wrench = 0,
  shotgun,
  plasma_rifle,
  nail_gun,
  // - //
  count
};

}

namespace AmmoTypes
{

enum evalue : AmmoType
{
  melee = 0,
  slugs,
  plasma,
  nails,
  // - //
  count
};

}

struct WeaponStats
{
  ProjectileType projectile     = 0;
  f32            rate_of_fire   = 0.0f;
  AmmoType       ammo           = AmmoTypes::melee;
  s32            projectile_cnt = 1; // per shot
  bool           hold_to_fire   = false;
  SoundID        shoot_snd      = INVALID_SOUND;
  f32            loudness_dist  = 20.0f;
};

struct WeaponAnim
{
  u16  frames_cnt   = 0; // how many frames in the attack animation there are
  u16  action_frame = 0; // in which frame of the attack is the projectile spawned
  f32  time         = 0; // how many seconds does the attack animation play
};

struct WeaponAnims
{
  cstr       set_name; // the name of the animation set
  WeaponAnim anims[2];
};

// We define these inside a .cpp where we register them into the cvar system
// so they can be tweaked in the game.
extern WeaponStats WEAPON_STATS[];
extern WeaponAnims WEAPON_ANIMS[];

}
