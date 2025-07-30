// Project Nucledian Source File
#pragma once

#include <SDL.h>
#include <types.h>
#include <engine/input/game_input.h>
#include <engine/entity/entity.h>
#include <engine/entity/entity.h>
#include <engine/graphics/camera.h>

#include <game/weapons_types.h>

#include <math/vector.h>

#include <engine/entity/entity_types.h>

namespace nc
{

class Player : public Entity
{
public:
  using Base = Entity;

  Player(vec3 position);

  static EntityType get_type_static();

  void get_wish_velocity(GameInputs input, f32 delta_seconds);
  void handle_inputs(GameInputs input, GameInputs prev_input);
  bool get_attack_state(GameInputs curInput, GameInputs prevInput, f32 delta_seconds);
  void apply_velocity(f32 delta_seconds);
  void damage(int damage);
  void die();

  Camera* get_camera();

  vec3 get_look_direction();
  f32 get_view_height();
  vec3& get_velocity();

  WeaponType get_equipped_weapon()         const;
  bool       has_weapon(WeaponType weapon) const;

private:
  void apply_acceleration(const nc::vec3& movement_direction, f32 delta_seconds);
  void apply_deceleration(const nc::vec3& movement_direction, f32 delta_seconds);

  //vec3 position;
  vec3 velocity = VEC3_ZERO; // forward/back - left/right velocity
  static inline f32 view_height = 0.5f;

  f32 MAX_SPEED = 5.0f;
  f32 ACCELERATION = 25.0f;
  f32 DECELERATION = 14.0f;
  f32 GRAVITY = 6.0f;

  vec3 m_forward = VEC3_ZERO;
  f32 angle_pitch = 0; //UP-DOWN
  f32 angle_yaw = 0; //LET-RIGHT

  /*PlayerSpecificInputs lastInputs;
  PlayerSpecificInputs currentInputs;*/

  int maxHealth = 100;
  int currentHealth;

  // Bit flags for the weapons owned
  WeaponFlags owned_weapons  = 0;
  WeaponType  current_weapon = 0;

  bool alive = true;

  Camera camera;
};

}