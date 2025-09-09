// Project Nucledian Source File
#pragma once
#include <engine/entity/entity.h>
#include <engine/entity/entity.h>
#include <engine/entity/entity_types.h>
#include <engine/appearance.h>
#include <transform.h>

namespace nc
{

enum class EnemyState
{
  idle = 0, // stands on a spot, does not move
  alert,    // has a target, follows it and attacks it if visible
  dead,     // dead, does not do anything
  count
};

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
  void handle_attack();
  void handle_movement(f32 delta);
  void die();

  void handle_idle(f32 delta);
  void handle_alert(f32 delta);

  bool can_see_point(vec3 pt) const;
  bool can_attack()           const;

private:
  vec3       velocity;
  vec3       facing;
  EnemyState state = EnemyState::idle;
  Appearance appear;

  struct Path
  {
    std::vector<vec3> points;
  };

  Path     current_path;
  EntityID target_id             = INVALID_ENTITY_ID;
  vec3     follow_target_pos     = VEC3_ZERO;
  f32      time_since_saw_target = 0.0f;
  bool     can_see_target : 1    = false;

  int health;
  int maxHealth;

  float attackDelay = 3.0f;
  float time_until_attack = 3.0f;
};

}
