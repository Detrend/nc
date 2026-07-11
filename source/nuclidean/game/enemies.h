// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <common.h>
#include <game/game_types.h>
#include <game/projectiles.h>

#include <metaprogramming.h> // ARRAY_LENGTH
#include <anim_state_machine.h>
#include <engine/database/database.h>

#include <array>

namespace nc
{

// We want to distinguish between animation states and enemy states.
// For example, we might want an enemy that is in the idle state to walk back
// to his spawn point, in which case he would have the "walk" anim state and
// "idle" state
namespace ActorAnimStates
{

enum evalue : ActorAnimState
{
  idle,
  walk,
  attack,
  dead,
  dying, // animation of dying
  // - //
  count,
};

}

static constexpr cstr ACTOR_ANIM_STATES_NAMES[]
{
  "idle",
  "walk",
  "attack",
  "dead",
  "dying",
};
static_assert(ARRAY_LENGTH(ACTOR_ANIM_STATES_NAMES) == ActorAnimStates::count);

namespace ActorAnimStatesFlags
{

static_assert(ActorAnimStates::count <= 8, "Overflow of u8 for state flags.");
enum evalue : ActorAnimStateFlag
{
  idle   = 1 << ActorAnimStates::idle,
  walk   = 1 << ActorAnimStates::walk,
  attack = 1 << ActorAnimStates::attack,
  dead   = 1 << ActorAnimStates::dead,
  dying  = 1 << ActorAnimStates::dying,
  // - //
  none   = 0,
  all    = cast<u8>(-1),
};

}

constexpr ActorAnimStateFlag actor_anim_state_to_flag(ActorAnimState state)
{
  return 1 << state;
}

using GotoArray = std::array<ActorAnimState, ActorAnimStates::count>;
static constexpr GotoArray ACTOR_ANIM_TRANSITIONS
{
  ActorAnimStates::idle, // idle   -> idle
  ActorAnimStates::walk, // walk   -> walk
  ActorAnimStates::idle, // attack -> idle
  ActorAnimStates::dead, // dead   -> dead
  ActorAnimStates::dead, // dying  -> dead
};
using ActorFSM = AnimFSM<ActorAnimStates::count, ACTOR_ANIM_TRANSITIONS>;

struct EnemyStats
{
  DbCol<f32              , "move speed"         > move_speed     = 5.0f;
  DbCol<s32              , "max hp"             > max_hp         = 100;
  DbCol<f32              , "height"             > height         = 2.0f;
  DbCol<f32              , "eye height"         > eye_height     = 1.8f;
  DbCol<f32              , "attack radius"      > atk_height     = 1.5f;
  DbCol<f32              , "radius"             > radius         = 0.25f;
  DbCol<f32              , "min attack delay"   > atk_delay_min  = 3.0f;
  DbCol<f32              , "max attack delay"   > atk_delay_max  = 8.0f;
  DbCol<u32              , "attack frame"       > attack_frame   = 0;
  DbCol<bool             , "is melee"           > is_melee       = false;
  DbCol<f32              , "infight chance"     > infight_chance = 0.0f;
  DbCol<f32              , "step height"        > step_height    = 1.0f;
  DbCol<std::vector<u32> , "state sprite count" > state_sprite_cnt;
  DbCol<std::vector<f32> , "state sprite length"> state_sprite_len;
  DbCol<ProjectileTypes::evalue, "projectile"   > projectile     = ProjectileTypes::wrench;
};

}


namespace nc::db
{

inline Database<EnemyStats> enemies("enemy");

}
