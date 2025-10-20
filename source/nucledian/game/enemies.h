// Project Nucledian Source File
#pragma once

#include <types.h>
#include <common.h>
#include <game/game_types.h>

#include <metaprogramming.h> // ARRAY_LENGTH
#include <anim_state_machine.h>

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

static constexpr ActorAnimStateFlag ActorAnimStateToFlag(ActorAnimState state)
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
  u32            state_sprite_cnt[ActorAnimStates::count]{};
  f32            state_sprite_len[ActorAnimStates::count]{};
  u32            attack_frame  = 0;
  bool           is_melee : 1  = false;
};

extern EnemyStats ENEMY_STATS[];

}
