// Project Nucledian Source File
#pragma once

#include <engine/entity/entity.h>
#include <engine/entity/entity_types.h>

#include <engine/appearance.h>
#include <anim_state_machine.h>

#include <game/game_types.h> // EnemyType

namespace nc
{

using ActorAnimState     = u8;
using ActorAnimStateFlag = u8;

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

static constexpr ActorAnimStateFlag AnimStateToFlag(ActorAnimState state)
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

struct EnemyStats;

class Enemy : public Entity
{
public:
  static EntityType get_type_static();

  Enemy(vec3 position, vec3 facing, EnemyType type);

  void update(f32 delta);
  void damage(int damage, EntityID from_who);

  vec3&             get_velocity();
  Appearance&       get_appearance();
  const Appearance& get_appearance() const;
  const EnemyStats& get_stats()      const;

private:
  void handle_ai(f32 delta);
  void handle_movement(f32 delta);
  void handle_appearance(f32 delta);
  void die();
  void on_dying_anim_end();

  void handle_ai_idle(f32 delta);
  void handle_ai_alert(f32 delta);

  bool can_see_point(vec3 pt) const;
  bool can_attack()           const;

private:
  enum class EnemyAiState : u8
  {
    idle = 0, // stands on a spot, does not move
    alert,    // has a target, follows it and attacks it if visible
    dead,     // dead, does not do anything
    count
  };

  enum TriggerTypes : u8
  {
    trigger_fire,
    trigger_die,  // TODO
  };

  struct Path
  {
    std::vector<vec3> points;
  };

  // Global only temporaly, this will be set per enemy type.
  static constexpr ActorAnimStateFlag dir8_states
    = ActorAnimStatesFlags::all
    & ~(ActorAnimStatesFlags::dying | ActorAnimStatesFlags::dead);

  EnemyType    type                  = 0;
  vec3         velocity              = VEC3_ZERO;
  vec3         facing                = VEC3_ZERO;
  EnemyAiState state                 = EnemyAiState::idle;
  Appearance   appear;
  ActorFSM     anim_fsm;
  Path         current_path;
  EntityID     target_id             = INVALID_ENTITY_ID;
  vec3         follow_target_pos     = VEC3_ZERO;
  f32          time_since_saw_target = 0.0f;
  f32          time_since_idle       = 0.0f;
  bool         can_see_target : 1    = false;

  int health;

  float attackDelay = 3.0f;
  float time_until_attack = 3.0f;
};

}
