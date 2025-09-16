// Project Nucledian Source File
#pragma once

#include <engine/entity/entity.h>
#include <engine/entity/entity_types.h>

#include <engine/appearance.h>
#include <anim_state_machine.h>

namespace nc
{

class Enemy : public Entity
{
public:
  static EntityType get_type_static();

  Enemy(vec3 position, vec3 facing);

  void update(f32 delta);
  void damage(int damage);

  vec3&             get_velocity();
  Appearance&       get_appearance();
  const Appearance& get_appearance() const;

private:
  void handle_ai(f32 delta);
  void handle_movement(f32 delta);
  void handle_appearance(f32 delta);
  void die();

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

  // We want to distinguish between animation states and enemy states.
  // For example, we might want an enemy that is in the idle state to walk back
  // to his spawn point, in which case he would have the "walk" anim state and
  // "idle" state
  enum AnimStates : u8
  {
    idle,
    walk,
    attack,
    dead,
    // - //
    count,
  };

  using GotoArray = std::array<AnimStates, AnimStates::count>;
  static constexpr GotoArray ANIM_TRANSITIONS
  {
    AnimStates::idle, // idle   -> idle
    AnimStates::walk, // walk   -> walk
    AnimStates::idle, // attack -> idle
    AnimStates::dead, // dead   -> dead
  };
  using EnemyFSM = AnimFSM<AnimStates::count, ANIM_TRANSITIONS>;

  static constexpr cstr ANIM_STATES_NAMES[]
  {
    "idle",
    "walk",
    "attack",
    "dead",
  };
  static_assert(ARRAY_LENGTH(ANIM_STATES_NAMES) == AnimStates::count);

  struct Path
  {
    std::vector<vec3> points;
  };

  vec3         velocity              = VEC3_ZERO;
  vec3         facing                = VEC3_ZERO;
  EnemyAiState state                 = EnemyAiState::idle;
  Appearance   appear;
  EnemyFSM     anim_fsm;
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
