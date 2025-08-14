// Project Nucledian Source File
#pragma once

#include <types.h>

#include <engine/entity/entity.h>
#include <engine/graphics/camera.h>

#include <game/weapons_types.h>
#include <engine/entity/entity_types.h>

#include <math/vector.h>

#include <anim_state_machine.h>
#include <array>


namespace nc
{
// Forward declare, we do not want to include
struct GameInputs;
struct RenderGunProperties;
}

namespace nc
{

class Player : public Entity
{
public:
  using Base = Entity;

  Player(vec3 position);

  static EntityType get_type_static();

  void damage(int damage);
  void die();

  void update(GameInputs input, GameInputs prev_input, f32 delta_seconds);

  Camera* get_camera();
  int     get_health();


  vec3  get_look_direction();
  f32   get_view_height();
  vec3& get_velocity();

  WeaponType get_equipped_weapon()         const;
  bool       has_weapon(WeaponType weapon) const;

  void get_gun_props(RenderGunProperties& props) const;

private:
  void apply_velocity(f32 delta_seconds);
  void handle_attack(GameInputs input, GameInputs prev_input, f32 delta_seconds);
  void handle_weapon_change(GameInputs input, GameInputs prev_input);
  void calculate_wish_velocity(GameInputs input, f32 delta_seconds);
  bool get_attack_state(GameInputs curInput, GameInputs prevInput, f32 delta_seconds);
  void apply_acceleration(vec3 movement_direction, f32 delta_seconds);
  void apply_deceleration(vec3 movement_direction, f32 delta_seconds);
  void update_gun_sway(f32 delta);
  void update_camera(f32 delta);
  void update_gun_anim(f32 delta);
  void do_attack();
  void change_weapon(WeaponType new_weapon);

  //vec3 position;
  vec3 velocity = VEC3_ZERO; // forward/back - left/right velocity
  static inline f32 view_height = 0.5f;

  vec3 forward     = VEC3_ZERO;
  f32  angle_pitch = 0.0f; //UP-DOWN
  f32  angle_yaw   = 0.0f; //LET-RIGHT

  /*PlayerSpecificInputs lastInputs;
  PlayerSpecificInputs currentInputs;*/

  s32 current_health;

  // Camera spring
  f32 vertical_camera_offset = 0.0f;

  // Gun sway
  f32 moving_time           = 0.0f; // for how long we were moving
  f32 time_since_gun_change = 0.0f;
  f32 time_since_shoot      = 0.0f;
  f32 time_since_start      = 0.0f;
  f32 air_time              = 0.0f;

  // Bit flags for the weapons owned
  WeaponFlags owned_weapons  = 0;
  WeaponType  current_weapon = 0;

  bool alive     : 1 = true;
  bool on_ground : 1 = true;

  enum WeaponStates : u8
  {
    idle,
    attack,
    // - //
    count
  };

  using GotoArray = std::array<WeaponStates, WeaponStates::count>;
  static constexpr GotoArray WEAPON_TRANSITIONS
  {
    WeaponStates::idle, // idle->idle
    WeaponStates::idle  // fire->idle
  };

  using WeaponAnimFSM = AnimFSM<WeaponStates::count, WEAPON_TRANSITIONS>;
  WeaponAnimFSM weapon_fsm;

  Camera camera;
};

}