// Project Nucledian Source File
#include <game/weapons.h>
#include <game/projectiles.h>

#include <metaprogramming.h>
#include <engine/sound/sound_resources.h>

#include <cvars.h>

namespace nc
{

// ========================================================================== //
//                          Weapon Stats                                      //
// ========================================================================== //
WeaponStats WEAPON_STATS[] = 
{
  // Wrench
  WeaponStats
  {
    .projectile = ProjectileTypes::wrench,
    .rate_of_fire = 1.0f, .ammo = AmmoTypes::melee,
    .projectile_cnt = 1, .hold_to_fire = false,
    .shoot_snd = Sounds::hurt,
    .loudness_dist = 0.0f,
  },

  // Shotgun
  WeaponStats
  {
    .projectile = ProjectileTypes::shotgun_slug,
    .rate_of_fire = 1.0f, .ammo = AmmoTypes::slugs,
    .projectile_cnt = 10, .hold_to_fire = false,
    .shoot_snd = Sounds::shotgun,
    .loudness_dist = 30.0f,
    .spread_amount = 0.15f,
  },

  // Plasma rifle
  WeaponStats
  {
    .projectile = ProjectileTypes::plasma_ball,
    .rate_of_fire = 0.5f, .ammo = AmmoTypes::plasma,
    .projectile_cnt = 1, .hold_to_fire = true,
    .shoot_snd = Sounds::plasma_rifle,
    .loudness_dist = 20.0f,
  },

  // Nail gun
  WeaponStats
  {
    .projectile = ProjectileTypes::plasma_ball,
    .rate_of_fire = 8.0f, .ammo = AmmoTypes::nails,
    .projectile_cnt = 2, .hold_to_fire = true
  },
};

static_assert
(
  ARRAY_LENGTH(WEAPON_STATS) == WeaponTypes::count,
  "If this asserts then you probably added a new weapon, "
  "but did not define it's stats."
);

// ========================================================================== //
//                          Weapon Anims                                      //
// ========================================================================== //
WeaponAnims WEAPON_ANIMS[] =
{
  // Wrench
  WeaponAnims
  {
    .set_name = "wrench",
    .anims =
    {
      // Idle
      WeaponAnim
      {
        .frames_cnt = 1, .time = 1.0f
      },
      // Attack
      WeaponAnim
      {
        .frames_cnt = 19, .action_frame = 5, .time = 0.8f
      },
    },
  },

  // Shotgun
  WeaponAnims
  {
    .set_name = "shotgun",
    .anims = 
    {
      // Idle
      WeaponAnim
      {
        .frames_cnt = 1, .time = 1.0f
      },
      // Attack
      WeaponAnim
      {
        .frames_cnt = 41, .action_frame = 1, .time = 2.0f
      },
    },
  },

  // Plasma rifle
  WeaponAnims
  {
    .set_name = "plasma_gun",
    .anims = 
    {
      // Idle
      WeaponAnim
      {
        .frames_cnt = 1, .time = 1.0f
      },
      // Attack
      WeaponAnim
      {
        .frames_cnt = 9, .action_frame = 1, .time = 0.4f
      },
    },
  },

  // Nail gun
  WeaponAnims{},
};

}

// ====================================================
//                Cvar Registrations
// ====================================================
#define NC_REG_WP(_c, _d, _mn, _mx, _l) NC_REGISTER_CVAR_EXTERNAL_CPP_RANGED_IMPL(_c, _mn, _mx, _d, _l)

#define NC_REGISTER_WEAPON_STATS(_weapon, _member)                                                 \
  inline ::nc::WeaponStats& _weapon = ::nc::WEAPON_STATS[::nc::WeaponTypes:: _member];             \
  NC_REG_WP(_weapon .rate_of_fire,   "Rate of fire",     0, 20, NC_TOKENJOIN(__LINE__, PA));       \
  NC_REG_WP(_weapon .projectile_cnt, "Projectile count", 0, 16, NC_TOKENJOIN(__LINE__, PB));       \
  NC_REG_WP(_weapon .hold_to_fire,   "Hold to fire",     0, 1,  NC_TOKENJOIN(__LINE__, PC));       \
  NC_REG_WP(_weapon .spread_amount,  "Spread",           0, 5,  NC_TOKENJOIN(__LINE__, PD));

#define NC_REGISTER_WEAPON_STATS_EXT(_w) \
  NC_REGISTER_WEAPON_STATS( _w##_weapon, _w)

NC_REGISTER_WEAPON_STATS_EXT(wrench)
NC_REGISTER_WEAPON_STATS_EXT(shotgun)
NC_REGISTER_WEAPON_STATS_EXT(plasma_rifle)
NC_REGISTER_WEAPON_STATS_EXT(nail_gun)
