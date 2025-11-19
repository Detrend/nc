// Project Nucledian Source File
#pragma once

#include <engine/entity/entity.h>
#include <engine/entity/entity_types.h>

#include <engine/map/map_types.h>
#include <engine/appearance.h>
#include <anim_state_machine.h>

#include <game/game_types.h> // EnemyType
#include <game/enemies.h>

namespace nc
{

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

  vec3 get_eye_pos() const;
  vec3 get_facing()  const;

private:
  void handle_ai(f32 delta);
  void handle_movement(f32 delta);
  void handle_appearance(f32 delta);
  void die();
  void on_dying_anim_end();

  // Returns true if this enemy can perform a visibility query this frame.
  bool is_my_turn_for_visibility_query() const;

  void handle_ai_idle(f32 delta);
  void handle_ai_alert(f32 delta);

  bool can_see_point(vec3 pt, vec3 look_dir) const;
  bool can_attack(const Entity& target)      const;

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

  EnemyType    type                  = 0;
  vec3         velocity              = VEC3_ZERO;
  vec3         facing                = VEC3_ZERO;
  EnemyAiState state                 = EnemyAiState::idle;
  Appearance   appear;
  ActorFSM     anim_fsm;
  Path         current_path;
  EntityID     target_id             = INVALID_ENTITY_ID;
  ActivatorID  on_death_trigger      = INVALID_ACTIVATOR_ID;
  vec3         follow_target_pos     = VEC3_ZERO;
  f32          time_since_saw_target = 0.0f;
  f32          time_since_idle       = 0.0f;
  bool         can_see_target : 1    = false;

  int health;

  float attackDelay = 3.0f;
  float time_until_attack = 3.0f;
};

}
