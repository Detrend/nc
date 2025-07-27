// Project Nucledian Source File

#include <types.h>

#include <game/weapons_types.h>

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

struct ProjectileStats
{
  f32 dmg_falloff = 0; // per meter travelled
  f32 speed       = 0; // m/s
  f32 radius      = 0; // m
  f32 gravity     = 0; // how much to fall
  f32 friction    = 1; // slowdown per collision, [0-1] range, 1 = none
  f32 lifetime    = 0; // how long alive in seconds, 0 for inf
  s32 damage      = 0; // units
  s32 bounce_cnt  = 0; // if should bounce and how many times, 0 = death on hit
};

struct WeaponStats
{
  f32      rate_of_fire     = 0.0f;
  AmmoType ammo             = AmmoTypes::melee;
  s32      projectile_cnt   = 1; // per shot
  bool     hold_to_fire     = false;
};


// We define these inside a .cpp where we register them into the cvar system
// so they can be tweaked in game.
extern ProjectileStats PROJECTILE_STATS[];
extern WeaponStats     WEAPON_STATS    [];

}
