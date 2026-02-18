// Project Nucledian Source File
#include <game/projectiles.h>

#include <metaprogramming.h>

#include <cvars.h>

namespace nc
{

// ========================================================================== //
//                       Projectile Stats                                     //
// ========================================================================== //
ProjectileStats PROJECTILE_STATS[] = 
{
  // Wrench
  ProjectileStats
  {
    .damage = 25,
  },

  // Plasma ball
  ProjectileStats
  {
    .dmg_falloff = 0.05f, .speed = 20.0f, .radius = 0.2f, .damage = 25,
    .bounce_cnt = 3, .sprite = "plasma_ball", .sprite_cnt = 1,
    .light_color = colors::BLUE,
    .hit_sprite = "plasma_ball_death",
    .hit_sprite_cnt = 6,
    .hit_sprite_len = 0.5f,
    .hit_sprite_col = colors::BLUE,
    .hit_sprite_rad = 2.0f,
  },

  // Shotgun slug
  ProjectileStats
  {
    .dmg_falloff = 0.05f, .speed = 100.0f, .radius = 0.01f, .damage = 15,
    .bounce_cnt = 0, .sprite = nullptr, .sprite_cnt = 1,
    .light_color = VEC3_ZERO,
    .hit_sprite = "buck",
    .hit_sprite_cnt = 4,
    .hit_sprite_len = 0.35f,
  },

  // Fire ball
  ProjectileStats
  {
    .dmg_falloff = 0.05f, .speed = 12.0f, .radius = 0.2f, .damage = 12,
    .sprite = "cultist_projectile", .sprite_cnt = 4, .anim_len = 0.8f,
    .light_color = colors::RED * 0.65f,
  },

  // Possessed fist
  ProjectileStats
  {
    .damage = 30,
    .sprite = "cultist_projectile",
  },
};

static_assert
(
  ARRAY_LENGTH(PROJECTILE_STATS) == ProjectileTypes::count,
  "If this asserts then you probably added a new projectile, "
  "but did not define it's projectile stats."
);

}

// ====================================================
//                Cvar Registrations
// ====================================================
#define NC_REG_PROJ(_c, _d, _mn, _mx, _l) NC_REGISTER_CVAR_EXTERNAL_CPP_RANGED_IMPL(_c, _mn, _mx, _d, _l)

#define NC_REGISTER_PROJECTILE_STATS(_projectile, _member)                                               \
  inline ::nc::ProjectileStats& _projectile = ::nc::PROJECTILE_STATS[::nc::ProjectileTypes:: _member];   \
  NC_REG_PROJ(_projectile .dmg_falloff, "Damage Falloff",           0, 100, NC_TOKENJOIN(__LINE__, WA)); \
  NC_REG_PROJ(_projectile .speed,       "Projectile speed (m/s)",   0, 100, NC_TOKENJOIN(__LINE__, WB)); \
  NC_REG_PROJ(_projectile .radius,      "Radius (m)",               0, 0.5, NC_TOKENJOIN(__LINE__, WC)); \
  NC_REG_PROJ(_projectile .gravity,     "Gravity",                  0, 1,   NC_TOKENJOIN(__LINE__, WD)); \
  NC_REG_PROJ(_projectile .friction,    "Friction [0.0-1.0]",       0, 1,   NC_TOKENJOIN(__LINE__, WE)); \
  NC_REG_PROJ(_projectile .lifetime,    "Lifetime (s)",             0, 100, NC_TOKENJOIN(__LINE__, WF)); \
  NC_REG_PROJ(_projectile .damage,      "Damage (units)",           0, 100, NC_TOKENJOIN(__LINE__, WG)); \
  NC_REG_PROJ(_projectile .bounce_cnt,  "Bounce times (0 = don't)", 0, 10,  NC_TOKENJOIN(__LINE__, WH));

#define NC_REGISTER_PROJECTILE_STATS_EXT(_w) \
  NC_REGISTER_PROJECTILE_STATS(_w##_projectile, _w)

NC_REGISTER_PROJECTILE_STATS_EXT(wrench)
NC_REGISTER_PROJECTILE_STATS_EXT(plasma_ball)
NC_REGISTER_PROJECTILE_STATS_EXT(fire_ball)
