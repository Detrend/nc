// Project Nucledian Source File
#include <game/weapons.h>
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
    .rate_of_fire = 1.0f, .ammo = AmmoTypes::melee,
    .projectile_cnt = 1, .hold_to_fire = false,
    .shoot_snd = Sounds::hurt,
  },

  // Shotgun
  WeaponStats
  {
    .rate_of_fire = 1.0f, .ammo = AmmoTypes::slugs,
    .projectile_cnt = 5, .hold_to_fire = false
  },

  // Plasma rifle
  WeaponStats
  {
    .rate_of_fire = 0.5f, .ammo = AmmoTypes::plasma,
    .projectile_cnt = 1, .hold_to_fire = true,
    .shoot_snd = Sounds::plasma_rifle,
  },

  // Nail gun
  WeaponStats
  {
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
  WeaponAnims{},

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

// ========================================================================== //
//                       Projectile Stats                                     //
// ========================================================================== //
ProjectileStats PROJECTILE_STATS[] = 
{
  // Wrench
  ProjectileStats{.damage = 10}, 

  // Shotgun
  ProjectileStats{.dmg_falloff = 0.2f, .speed = 12.0f, .radius = 0.01f, .damage = 8}, 

  // Plasma rifle
  ProjectileStats{.dmg_falloff = 0.05f, .speed = 12.0f, .radius = 0.2f, .damage = 12, .bounce_cnt = 3}, 

  // Nail gun
  ProjectileStats{.dmg_falloff = 0.05f, .speed = 8.0f, .radius = 0.03f, .damage = 3},
};

static_assert
(
  ARRAY_LENGTH(PROJECTILE_STATS) == WeaponTypes::count,
  "If this asserts then you probably added a new weapon, "
  "but did not define it's projectile stats."
);

}

// ====================================================
//                Cvar Registrations
// ====================================================
#define NC_REG_WP(_c, _d, _mn, _mx, _l) NC_REGISTER_CVAR_EXTERNAL_CPP_RANGED_IMPL(_c, _mn, _mx, _d, _l)

#define NC_REGISTER_WEAPON_STATS(_weapon, _member)                                                 \
  inline ::nc::WeaponStats& _weapon = ::nc::WEAPON_STATS[::nc::WeaponTypes:: _member];             \
  NC_REG_WP(_weapon .rate_of_fire,   "Rate of fire",     0, 20, NC_TOKENJOIN(__LINE__, PA));       \
  NC_REG_WP(_weapon .projectile_cnt, "Projectile count", 0, 16, NC_TOKENJOIN(__LINE__, PB));       \
  NC_REG_WP(_weapon .hold_to_fire,   "Hold to fire",     0, 1,  NC_TOKENJOIN(__LINE__, PC));

#define NC_REGISTER_PROJECTILE_STATS(_weapon, _member)                                             \
  inline ::nc::ProjectileStats& _weapon = ::nc::PROJECTILE_STATS[::nc::WeaponTypes:: _member];     \
  NC_REG_WP(_weapon .dmg_falloff, "Damage Falloff",           0, 100, NC_TOKENJOIN(__LINE__, WA)); \
  NC_REG_WP(_weapon .speed,       "Projectile speed (m/s)",   0, 100, NC_TOKENJOIN(__LINE__, WB)); \
  NC_REG_WP(_weapon .radius,      "Radius (m)",               0, 0.5, NC_TOKENJOIN(__LINE__, WC)); \
  NC_REG_WP(_weapon .gravity,     "Gravity",                  0, 1,   NC_TOKENJOIN(__LINE__, WD)); \
  NC_REG_WP(_weapon .friction,    "Friction [0.0-1.0]",       0, 1,   NC_TOKENJOIN(__LINE__, WE)); \
  NC_REG_WP(_weapon .lifetime,    "Lifetime (s)",             0, 100, NC_TOKENJOIN(__LINE__, WF)); \
  NC_REG_WP(_weapon .damage,      "Damage (units)",           0, 100, NC_TOKENJOIN(__LINE__, WG)); \
  NC_REG_WP(_weapon .bounce_cnt,  "Bounce times (0 = don't)", 0, 10,  NC_TOKENJOIN(__LINE__, WH));

#define NC_REGISTER_WEAPON_PROJECTILE_STATS(_w)     \
  NC_REGISTER_WEAPON_STATS(    _w##_weapon,     _w) \
  NC_REGISTER_PROJECTILE_STATS(_w##_projectile, _w)

NC_REGISTER_WEAPON_PROJECTILE_STATS(wrench)
NC_REGISTER_WEAPON_PROJECTILE_STATS(shotgun)
NC_REGISTER_WEAPON_PROJECTILE_STATS(plasma_rifle)
NC_REGISTER_WEAPON_PROJECTILE_STATS(nail_gun)
