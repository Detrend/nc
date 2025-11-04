// Project Nucledian Source File
#include <cvars.h>
#include <common.h>

#include <engine/core/engine.h>

#include <engine/player/player.h>
#include <engine/input/game_input.h>
#include <engine/input/input_system.h>

#include <engine/map/map_system.h>
#include <engine/map/physics.h>
#include <engine/player/thing_system.h>
#include <engine/entity/entity_system.h>

#include <engine/graphics/graphics_system.h> // RenderGunProperties
#include <engine/graphics/lights.h>

// Sound
#include <engine/sound/sound_system.h>
#include <engine/sound/sound_resources.h>

// Game
#include <game/projectile.h>
#include <game/weapons.h>
#include <game/item.h>
#include <game/entity_attachment_manager.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <engine/entity/entity_type_definitions.h>

#include <profiling.h>

#include <algorithm> // std::clamp
#include <format>    // std::format

namespace nc
{

//==============================================================================
static constexpr cstr WEAPON_STATE_NAMES[] = 
{
  "idle",
  "attack",
};

//==============================================================================
static void smooth_towards(f32& value, f32 target, f32 delta)
{
  nc_assert(delta >= 0.0f);

  if (std::abs(value - target) < delta)
  {
    value = target;
  }
  else
  {
    f32 sg = value > target ? -1.0f : 1.0f;
    value += delta * sg;
  }
}

//==============================================================================
constexpr WeaponFlags DEFAULT_WEAPONS
= (1 << WeaponTypes::plasma_rifle)
| (1 << WeaponTypes::wrench);
constexpr f32 PLAYER_HEIGHT     = 1.8f;
constexpr f32 PLAYER_EYE_HEIGHT = 1.65f;
constexpr f32 PLAYER_RADIUS     = 0.25f;

//==============================================================================
Player::Player(vec3 position, vec3 forward)
: Base(position, PLAYER_RADIUS, PLAYER_HEIGHT)
, angle_yaw(atan2f(forward.z, -forward.x) + HALF_PI) // MR says: no idea if it's ok
, current_health(CVars::player_max_hp)
, owned_weapons(DEFAULT_WEAPONS)
, current_weapon(WeaponTypes::wrench)
, weapon_fsm(0)
{
  this->camera.update_transform(position, angle_yaw, angle_pitch, view_height);

  // Has to be called to set-up the FSM
  this->change_weapon(this->get_equipped_weapon());
}

//==============================================================================
void Player::calculate_wish_velocity(GameInputs input, f32 delta_seconds)
{
  // INPUT HANDLING
  vec3 input_vector = vec3(0, 0, 0);

  // movement keys
  if (input.player_inputs.keys & (1 << PlayerKeyInputs::forward))
  {
    input_vector += vec3(0, 0, 1);
  }
  if (input.player_inputs.keys & (1 << PlayerKeyInputs::backward))
  {
    input_vector += vec3(0, 0, -1);
  }
  if (input.player_inputs.keys & (1 << PlayerKeyInputs::left))
  {
    input_vector += vec3(-1, 0, 0);
  }
  if (input.player_inputs.keys & (1 << PlayerKeyInputs::right))
  {
    input_vector += vec3(1, 0, 0);
  }

  this->angle_pitch += input.player_inputs.analog[PlayerAnalogInputs::look_vertical];
  this->angle_yaw   += input.player_inputs.analog[PlayerAnalogInputs::look_horizontal];

  this->angle_yaw   = rem_euclid(this->angle_yaw, 2.0f * PI);
  this->angle_pitch = clamp(this->angle_pitch, -HALF_PI + 0.001f, HALF_PI - 0.001f);

  if (CVars::lock_camera_pitch)
  {
    // set the pitch permanently to 0 if the camera is locked
    angle_pitch = 0.0f;
  }

  quat y_transform = angleAxis(this->angle_yaw,   VEC3_Y);
  quat x_transform = angleAxis(this->angle_pitch, VEC3_X);
  this->forward = y_transform * x_transform * FRONT_DIR;

  // APLICATION OF VELOCITY
  const vec3 forward_dir = with_y(this->forward, 0);
  const vec3 right_dir   = cross(this->forward, VEC3_Y);
  const vec3 movement_direction = normalize_or_zero
  (
    input_vector.x * right_dir
    + input_vector.y * VEC3_Y
    + input_vector.z * forward_dir
  );

  this->apply_deceleration(movement_direction, delta_seconds);
  this->apply_acceleration(movement_direction, delta_seconds);

  // JUMPING
  bool wants_jump = input.player_inputs.keys & (1 << PlayerKeyInputs::jump);
  bool can_jump   = this->on_ground;

  if (wants_jump && can_jump)
  {
    this->velocity.y = CVars::player_jump_force;
  }

  this->velocity.y -= CVars::player_gravity * delta_seconds;
}

//==========================================================================
void Player::handle_weapon_change(GameInputs input, GameInputs /*prev_input*/)
{
  for (WeaponType i = 0; i < WeaponTypes::count; ++i)
  {
    PlayerKeyFlags weapon_flag = 1 << (PlayerKeyInputs::weapon_0 + i);

    const bool owns_weapon  = this->has_weapon(i);
    const bool wants_weapon = input.player_inputs.keys & weapon_flag;

    if (owns_weapon && wants_weapon && current_weapon != i)
    {
      // Accept only the first one if multiple keys are pressed
      this->change_weapon(i);
      break;
    }
  }
}

//==============================================================================
bool Player::get_attack_state(GameInputs curr_input, GameInputs prev_input, f32)
{
  PlayerKeyFlags flag = (1 << PlayerKeyInputs::primary);
  bool holding_now  = curr_input.player_inputs.keys & flag;
  bool holding_prev = prev_input.player_inputs.keys & flag;

  bool can_hold = WEAPON_STATS[this->get_equipped_weapon()].hold_to_fire;

  if (holding_now && (can_hold || !holding_prev))
  {
      return true;
  }
  else
  {
    return false;
  }
}

//==============================================================================
void Player::apply_velocity(f32 delta_seconds)
{
  // Collide with everything but us and projectiles for now. Later, we will have
  // to enable collisions with projectiles once again.
  constexpr EntityTypeMask IGNORED_COLLIDERS
    = EntityTypeFlags::player
    | EntityTypeFlags::projectile
    | EntityTypeFlags::point_light
    | EntityTypeFlags::prop;

  constexpr EntityTypeMask PLAYER_COLLIDERS
    = PhysLevel::COLLIDE_ALL & ~IGNORED_COLLIDERS;

  // Report only pickups
  constexpr EntityTypeMask PLAYER_REPORTING
    = EntityTypeFlags::pickup | EntityTypeFlags::projectile;

  PhysLevel       lvl = ThingSystem::get().get_level();
  EntityRegistry& ecs = ThingSystem::get().get_entities();

  // Store the position here, change it, and then set it again later
  vec3 position = this->get_position();
  vec3 prev_pos = position;

  mat4 portal_transform = identity<mat4>();
  PhysLevel::CharacterCollisions collected_collisions;
  lvl.move_character
  (
    position, velocity, portal_transform, delta_seconds, PLAYER_RADIUS,
    PLAYER_HEIGHT, PLAYER_HEIGHT * 0.3f, PLAYER_COLLIDERS,
    PLAYER_REPORTING, &collected_collisions
  );

  // Change the position
  this->set_position(position);

  // Change the direction after portal transition
  this->forward = (portal_transform * vec4{this->forward, 0.0f}).xyz();

  // Recompute the angleYaw after moving through a portal
  const auto forward2 = normalize(with_y(forward, 0.0f));
  this->angle_yaw = rem_euclid
  (
    std::atan2f(forward2.z, -forward2.x) + HALF_PI, PI * 2
  );

  // Spring - makes sure that the camera moves smoothly on the stairs

  // We have to work with prev_pos that is relative to our portal transformation.
  //f32 prev_pos_rel_y = (portal_transform * vec4{prev_pos, 1.0f}).y;
  // Same as above but without unnecessary multiplications
  f32 prev_pos_rel_y
    = prev_pos.x * portal_transform[0].y
    + prev_pos.y * portal_transform[1].y
    + prev_pos.z * portal_transform[2].y
    + /*1.0f * */  portal_transform[3].y;

  f32 height_diff = position.y - prev_pos_rel_y;
  f32 max_offset  = CVars::camera_spring_height;
  this->vertical_camera_offset = clamp
  (
    this->vertical_camera_offset - height_diff, -max_offset, max_offset
  );

  // Set on ground if touching floor
  this->on_ground = !collected_collisions.floors.empty();

  // Process report only collisions
  for (EntityID report_id : collected_collisions.report_entities)
  {
    switch (report_id.type)
    {
      case EntityTypes::pickup:
      {
        PickUp* pickup = ecs.get_entity<PickUp>(report_id);
        nc_assert(pickup);

        if (pickup->pickup(*this))
        {
          // Destroy if picked up sucessfully.
          ecs.destroy_entity(report_id);
        }
        break;
      }

      case EntityTypes::projectile:
      {
        // TODO: later
        break;
      }
    }
  }
}

//==============================================================================
void Player::handle_attack(GameInputs curr_input, GameInputs prev_input, f32 dt)
{
  bool did_attack = this->get_attack_state(curr_input, prev_input, dt);

  if (did_attack && this->weapon_fsm.get_state() == WeaponStates::idle)
  {
    if (current_ammo[current_weapon] == -1)
    {
        this->weapon_fsm.set_state(WeaponStates::attack);
        return;
    }

    if (current_ammo[current_weapon] == 0)
    {
        return;
    }

    current_ammo[current_weapon] -= 1;
    this->weapon_fsm.set_state(WeaponStates::attack);
  }
}

//==============================================================================
void Player::apply_acceleration(vec3 movement_direction, f32 delta)
{
  f32 coeff = this->on_ground ? 1.0f : CVars::player_air_acc_coeff;
  velocity += movement_direction * CVars::player_acceleration * delta * coeff;

  // Speed cap
  if (length(vec2{velocity.x, velocity.z}) > CVars::player_max_speed)
  {
    f32 y = velocity.y;
    velocity.y = 0;
    velocity = normalize(velocity) * CVars::player_max_speed;
    velocity.y = y;
  }
}

//==============================================================================
void Player::apply_deceleration(vec3 /*movement_direction*/, f32 delta_seconds)
{
  // NOTE: Not sure what this is supposed to do, but it does not look ok.
  // vec3 reverse_velocity = -normalize_or_zero(with_y(velocity, 0.0f));
  // 
  // //apply deceleration if reverse key is pressed or if directional/axis key is not pressed
  // if (movement_direction.x == 0 || signbit(movement_direction.x) != signbit(velocity.x))
  // {
  //   velocity.x = velocity.x + (reverse_velocity.x * DECELERATION * delta_seconds);
  // }
  // 
  // if (movement_direction.z == 0 || signbit(movement_direction.z) != signbit(velocity.z))
  // {
  //   velocity.z = velocity.z + (reverse_velocity.z * DECELERATION * delta_seconds);
  // }

  // Apply general deceleration
  f32 coeff = this->on_ground ? 1.0f : CVars::player_air_dec_coeff;
  f32 reverse_len   = CVars::player_deceleration * delta_seconds * coeff;
  f32 velocity2_len = length(velocity.xz());

  f32  new_len = std::max(velocity2_len - reverse_len, 0.0f);
  vec3 new_hor = normalize_or_zero(with_y(velocity, 0.0f)) * new_len;
  velocity = with_y(new_hor, velocity.y);
}

//==============================================================================
void Player::update_gun_sway(f32 delta)
{
  const f32 move_fadein = CVars::gun_sway_move_fadein_time;
  const f32 air_fadein  = CVars::gun_sway_air_time;

  f32 velocity2_len    = length(this->velocity.xz());
  f32 max_velocity_len = CVars::player_max_speed;
  f32 velocity_amount  = velocity2_len / max_velocity_len;

  time_since_shoot      += delta;
  time_since_start      += std::sqrtf(velocity_amount) * delta;
  time_since_gun_change += delta;

  const bool is_moving = length(this->velocity.xz()) > 0.01f;
  const bool in_air    = !on_ground;

  smooth_towards(this->moving_time, is_moving ? move_fadein : 0.0f, delta);
  smooth_towards(this->air_time,    in_air    ? air_fadein  : 0.0f, delta);
}

//==============================================================================
void Player::update_camera(f32 delta)
{
  // Update the spring
  f32 spd = CVars::camera_spring_update_speed;
  smooth_towards(this->vertical_camera_offset, 0.0f, delta * spd);

  camera.update_transform
  (
    this->get_position(), this->angle_yaw, this->angle_pitch,
    PLAYER_EYE_HEIGHT + this->vertical_camera_offset
  );
}

//==============================================================================
void Player::update_gun_anim(f32 delta)
{
  if (this->current_weapon == INVALID_WEAPON_TYPE)
  {
    return;
  }

  using Trigger = WeaponAnimFSM::Trigger;
  using State   = WeaponAnimFSM::State;

  this->weapon_fsm.update(delta, [&](AnimFSMEvent event, Trigger, State)
  {
    if (event == AnimFSMEvents::trigger)
    {
      // Shoot
      this->do_attack();
    }
  });
}

//==============================================================================
void Player::do_attack()
{
  auto& entity_system = ThingSystem::get().get_entities();
  auto& sound_system  = SoundSystem::get();

  vec3 dir  = this->get_look_direction();
  vec3 from = this->get_position() + UP_DIR * PLAYER_EYE_HEIGHT + dir * 0.3f;

  WeaponType weapon = this->get_equipped_weapon();
  bool is_melee = WEAPON_STATS[weapon].ammo == AmmoTypes::melee;

  if (is_melee)
  {
    // TODO
  }
  else
  {
    // Spawn projectile
    Projectile* projectile = entity_system.create_entity<Projectile>
    (
      from, dir, this->get_id(), WEAPON_STATS[weapon].projectile
    );

    // And its light
    PointLight* light = entity_system.create_entity<PointLight>
    (
      from, 1.0f, 1.0f, 0.09f, 0.032f, colors::BLUE
    );

    // And attach it
    ThingSystem::get().get_attachment_mgr().attach_entity
    (
      light->get_id(), projectile->get_id(), EntityAttachmentFlags::all
    );
  }

  sound_system.play(WEAPON_STATS[weapon].shoot_snd, 0.5f);
  time_since_shoot = 0.0f;
}

//==============================================================================
void Player::change_weapon(WeaponType new_weapon)
{
  this->current_weapon        = new_weapon;
  this->time_since_gun_change = 0.0f;

  this->weapon_fsm.set_state(WeaponStates::idle);
  this->weapon_fsm.clear();

  const WeaponAnims& anim_set = WEAPON_ANIMS[this->current_weapon];

  for (u8 state : {WeaponStates::idle, WeaponStates::attack})
  {
    const WeaponAnim& anim = anim_set.anims[state];
    f32 time_per_frame = anim.time / anim.frames_cnt;
    f32 trigger_t      = anim.action_frame * time_per_frame;

    this->weapon_fsm.set_state_length(state, anim.time);
    this->weapon_fsm.add_trigger(state, trigger_t, 0);
  }
}

//==============================================================================
void Player::damage(int damage)
{
  this->current_health -= damage;

  if (this->current_health <= 0)
  {
    //this->die();
  }
}

//==============================================================================
void Player::die()
{
  alive = false;
}

//==============================================================================
int Player::get_health()
{
  return this->current_health;
}

//==============================================================================
Camera* Player::get_camera()
{
  return &this->camera;
}

//==============================================================================
void Player::update(GameInputs curr_input, GameInputs prev_input, f32 delta)
{
  NC_SCOPE_PROFILER(PlayerUpdate)

  if (!this->alive)
  {
    // Do nothing
    return;
  }

  this->update_gun_sway(delta);
  this->calculate_wish_velocity(curr_input, delta);
  this->handle_weapon_change(curr_input, prev_input);
  this->update_gun_anim(delta);
  this->handle_attack(curr_input, prev_input, delta);
  this->apply_velocity(delta);
  this->update_camera(delta); // should be after "apply_velocity"
}

//==============================================================================
vec3 Player::get_look_direction()
{
  //looking direction
  return angleAxis(angle_yaw, VEC3_Y) * angleAxis(angle_pitch, VEC3_X) * -VEC3_Z;
}

//==============================================================================
f32 Player::get_view_height()
{
  return this->view_height;
}

//==============================================================================
vec3& Player::get_velocity()
{
  return this->velocity;
}

//==============================================================================
WeaponType Player::get_equipped_weapon() const
{
  return this->current_weapon;
}

//==============================================================================
void Player::get_gun_props(RenderGunProperties& props_out) const
{
  const f32 max_gun_change = CVars::gun_change_time;
  const f32 sway_speed     = CVars::gun_sway_speed;
  const f32 sway_amount    = CVars::gun_sway_amount;
  const f32 fadein_time    = std::max(CVars::gun_sway_move_fadein_time, 0.001f);
  const f32 fadein_air     = std::max(CVars::gun_sway_air_time, 0.001f);

  // Normal sway
  f32 max_x = std::sin(sway_speed * time_since_start);
  f32 max_y = std::sin(sway_speed * time_since_start * 2.0f);

  // Gun change
  f32 gun_change_bounded = std::min(max_gun_change, time_since_gun_change);

  f32 gun_change_coeff = max_gun_change
    ? (gun_change_bounded / max_gun_change)
    : 0.0f;

  vec2 gun_change_offset = vec2{0.0f, (1.0f - gun_change_coeff)};

  // Movement
  f32 movement_coeff = moving_time / fadein_time;

  // Air
  f32 air_coeff = 1.0f - air_time / fadein_air;

  f32 total_coeff = movement_coeff * air_coeff;

  vec2 sway = vec2{max_x, max_y} * total_coeff * sway_amount + gun_change_offset;

  props_out.sway   = sway;

  if (this->current_weapon != INVALID_WEAPON_TYPE)
  {
    const WeaponAnims& anims = WEAPON_ANIMS[this->current_weapon];
    const WeaponStates state = cast<WeaponStates>(this->weapon_fsm.get_state());

    f32 anim_time  = this->weapon_fsm.get_time_relative();

    cstr set_name   = anims.set_name;
    cstr state_name = WEAPON_STATE_NAMES[state];
    u64  anim_frame = cast<u64>(anim_time * (anims.anims[state].frames_cnt));

    props_out.sprite = std::format("{}_{}_{}", set_name, state_name, anim_frame);
  }
}

//==============================================================================
bool Player::has_weapon(WeaponType weapon) const
{
  return owned_weapons & (1 << weapon);
}

//==============================================================================
EntityType Player::get_type_static()
{
  return EntityTypes::player;
}

//==============================================================================
int Player::get_current_weapon_ammo()
{
  return this->current_ammo[this->current_weapon];
}

}
