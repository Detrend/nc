// Project Nucledian Source File
#pragma once

#include <types.h>
#include <config.h>
#include <rng.h>

#include <engine/entity/entity.h>
#include <engine/graphics/camera.h>

#include <game/game_types.h>
#include <engine/entity/entity_types.h>

#include <math/vector.h>

#include <anim_state_machine.h>
#include <array>


namespace nc
{
// Forward declare, we do not want to include
struct PlayerSpecificInputs;
struct RenderGunProperties;
class  PickUp;
}

namespace nc
{

class Player : public Entity
{
public:
  using Base = Entity;

  Player(vec3 position, vec3 forward);

  static EntityType get_type_static();

  void damage(int damage);
  void heal(u32 how_much);
  void die();

  void update
  (
    PlayerSpecificInputs input,
    PlayerSpecificInputs prev_input,
    f32                  delta_seconds
  );

  Camera* get_camera();
  int     get_health()     const;
  int     get_max_health() const;

  s32     get_current_weapon_ammo();
  s32     get_ammo(WeaponType weapon)     const;
  s32     get_max_ammo(WeaponType weapon) const;

  vec3  get_look_direction();
  f32   get_view_height();
  vec3& get_velocity();

  WeaponType get_equipped_weapon()         const;
  bool       has_weapon(WeaponType weapon) const;
  void       give_weapon(WeaponType weapon);
  void       give_ammo(WeaponType weapon, u32 amount);

  void get_gun_props(RenderGunProperties& props) const;

#if NC_HOT_RELOAD
  void hot_reload_set_pos_rot(vec3 pos, f32 yaw, f32 pitch);
  void hot_reload_get_pos_rot(vec3& pos, f32& yaw, f32& pitch);
#endif

private:
  void apply_velocity(f32 delta_seconds);
  void handle_attack(PlayerSpecificInputs input, PlayerSpecificInputs prev_input, f32 delta_seconds);
  void handle_use(PlayerSpecificInputs input, PlayerSpecificInputs prev_input, f32 delta_seconds);
  void handle_weapon_change(PlayerSpecificInputs input, PlayerSpecificInputs prev_input);
  void calculate_wish_velocity(PlayerSpecificInputs input, f32 delta_seconds);
  void calculate_gravity_velocity(f32 delta_seconds);
  bool get_attack_state(PlayerSpecificInputs curInput, PlayerSpecificInputs prevInput, f32 delta_seconds);
  void apply_acceleration(vec3 movement_direction, f32 delta_seconds);
  void apply_deceleration(vec3 movement_direction, f32 delta_seconds);
  void update_gun_sway(f32 delta);
  void update_camera(f32 delta);
  void update_gun_anim(f32 delta);
  void do_attack();
  void change_weapon(WeaponType new_weapon);
  void handle_pickup(PickUp& pickup);

	// Alerts enemies close to the player
	void alert_nearby_enemies(f32 distance);

  vec3 velocity = VEC3_ZERO; // forward/back - left/right velocity
  static inline f32 view_height = 0.5f;

  vec3 forward     = VEC3_ZERO;
  f32  angle_pitch = 0.0f; //UP-DOWN
  f32  angle_yaw   = 0.0f; //LET-RIGHT

  s32 current_health;

  // Camera spring
  f32 vertical_camera_offset = 0.0f;

  f32 dead_camera_offset = 0.0f;

  // Gun sway
  f32 moving_time           = 0.0f; // for how long we were moving
  f32 time_since_gun_change = 0.0f;
  f32 time_since_shoot      = 0.0f;
  f32 time_since_start      = 0.0f;
  f32 air_time              = 0.0f;
  f32 time_since_death      = 0.0f;

  // Bit flags for the weapons owned
  WeaponFlags owned_weapons  = 0;
  WeaponType  current_weapon = 0;

  Rng rng;

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

  static constexpr s32 MAX_AMMO[4] = {-1, 20, 48, 96};

  s32 current_ammo[4] = {-1, 0, 30, 0}; // We start with nothing
  static_assert(ARRAY_LENGTH(current_ammo) == ARRAY_LENGTH(MAX_AMMO));
};

}