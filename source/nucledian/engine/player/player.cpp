// Project Nucledian Source File
#include <cvars.h>

#include <engine/core/engine.h>

#include <engine/player/player.h>
#include <engine/input/game_input.h>
#include <engine/input/input_system.h>

#include <engine/map/map_system.h>
#include <engine/map/physics.h>
#include <engine/player/thing_system.h>
#include <engine/entity/entity_system.h>

// Sound
#include <engine/sound/sound_system.h>
#include <engine/sound/sound_resources.h>

// Game
#include <game/projectile.h>
#include <game/weapons.h>
#include <game/item.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <engine/entity/entity_type_definitions.h>

#include <algorithm> // std::clamp

namespace nc
{

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

//==========================================================================
Player::Player(vec3 position)
  : Base(position, 0.25f, 0.5f)
  , owned_weapons((1 << WeaponTypes::plasma_rifle) | (1 << WeaponTypes::nail_gun))
  , current_weapon(WeaponTypes::plasma_rifle)
{
  currentHealth = maxHealth;
  camera.update_transform(position, angle_yaw, angle_pitch, view_height);
}

//==========================================================================
void Player::calculate_wish_velocity(GameInputs input, f32 delta_seconds)
{
  // INPUT HANDELING

  vec3 inputVector = vec3(0, 0, 0);

  if (!alive)
  {
    return;
  }

  // movement keys
  if (input.player_inputs.keys & (1 << PlayerKeyInputs::forward))
  {
    inputVector += vec3(0, 0, 1);
  }
  if (input.player_inputs.keys & (1 << PlayerKeyInputs::backward))
  {
    inputVector += vec3(0, 0, -1);
  }
  if (input.player_inputs.keys & (1 << PlayerKeyInputs::left))
  {
    inputVector += vec3(-1, 0, 0);
  }
  if (input.player_inputs.keys & (1 << PlayerKeyInputs::right))
  {
    inputVector += vec3(1, 0, 0);
  }

  angle_pitch += input.player_inputs.analog[PlayerAnalogInputs::look_vertical];
  angle_yaw += input.player_inputs.analog[PlayerAnalogInputs::look_horizontal];

  angle_yaw = rem_euclid(angle_yaw, 2.0f * PI);
  angle_pitch = clamp(angle_pitch, -HALF_PI + 0.001f, HALF_PI - 0.001f);

  if (CVars::lock_camera_pitch)
  {
    // set the pitch permanently to 0 if the camera is locked
    angle_pitch = 0.0f;
  }

  m_forward = angleAxis(angle_yaw, VEC3_Y) * angleAxis(angle_pitch, VEC3_X) * -VEC3_Z;

  // APLICATION OF VELOCITY
  const vec3 forward = with_y(m_forward, 0);
  const vec3 right = cross(forward, VEC3_Y);
  const vec3 movement_direction = normalize_or_zero
  (
    inputVector.x * right
    + inputVector.y * VEC3_Y
    + inputVector.z * forward
  );

  // JUMPING
  vec3 jump_force = vec3(0, 0, 0);
  bool wants_jump = input.player_inputs.keys & (1 << PlayerKeyInputs::jump);
  bool can_jump   = this->on_ground;

  if (wants_jump && can_jump)
  {
    // Jump!
    jump_force = vec3(0, 1, 0);
  }

  apply_deceleration(movement_direction, delta_seconds);

  apply_acceleration(movement_direction, delta_seconds);

  velocity += jump_force * 5.0f;
  velocity.y -= GRAVITY * delta_seconds;

  camera.update_transform(this->get_position(), angle_yaw, angle_pitch, view_height);
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
      current_weapon = i;
      time_since_gun_change = 0.0f;

      // Accept only the first one if multiple keys are pressed
      break; 
    }
  }
}

//==============================================================================
bool Player::get_attack_state(GameInputs curInput, GameInputs prevInput, [[maybe_unused]] f32 delta_seconds)
{
  if (curInput.player_inputs.keys & (1 << PlayerKeyInputs::primary) &&
    !(prevInput.player_inputs.keys & (1 << PlayerKeyInputs::primary)))
  {
    return true;
  }
  return false;
}

//==============================================================================
void Player::apply_velocity(f32 delta_seconds)
{
  // Collide with everything but us and projectiles for now. Later, we will have
  // to enable collisions with projectiles once again.
  constexpr EntityTypeMask PLAYER_COLLIDERS
    = PhysLevel::COLLIDE_ALL
    & ~(EntityTypeFlags::player | EntityTypeFlags::projectile);

  // Report only pickups
  constexpr EntityTypeMask PLAYER_REPORTING
    = EntityTypeFlags::pickup | EntityTypeFlags::projectile;

  PhysLevel       lvl = ThingSystem::get().get_level();
  EntityRegistry& ecs = ThingSystem::get().get_entities();

  // Store the position here, change it, and then set it again later
  vec3 position = this->get_position();

  PhysLevel::CharacterCollisions collected_collisions;
  lvl.move_character
  (
    position, velocity, &m_forward, delta_seconds, 0.25f, 1.0f,
    0.25f, PLAYER_COLLIDERS, PLAYER_REPORTING, &collected_collisions
  );

  // Change the position
  this->set_position(position);

  // Recompute the angleYaw after moving through a portal
  const auto forward2 = normalize(with_y(m_forward, 0.0f));
  this->angle_yaw = rem_euclid
  (
    std::atan2f(forward2.z, -forward2.x) + HALF_PI, PI * 2
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
  auto& entity_system = ThingSystem::get().get_entities();
  auto& sound_system  = SoundSystem::get();
  bool  did_attack    = this->get_attack_state(curr_input, prev_input, dt);

  if (did_attack)
  {
    vec3 dir  = this->get_look_direction();
    vec3 from = this->get_position() + UP_DIR * this->get_height() + dir * 0.3f;

    // Spawn projectile
    entity_system.create_entity<Projectile>
    (
      from, dir, true, this->get_equipped_weapon()
    );

    // play a sound
    // TODO: change the SFX
    sound_system.play(Sounds::plasma_rifle, 0.5f);

    time_since_shoot = 0.0f;
  }
}

//==============================================================================
void Player::apply_acceleration(vec3 movement_direction, [[maybe_unused]] f32 delta_seconds)
{
  velocity += movement_direction * ACCELERATION * delta_seconds;

  //speed cap
  //if (sqrtf(velocity.x * velocity.x + velocity.z * velocity.z) > MAX_SPEED)
  if (length(vec2(velocity.x, velocity.z)) > MAX_SPEED)
  {
    float y = velocity.y;
    velocity.y = 0;
    velocity = normalize_or_zero(velocity) * MAX_SPEED;
    velocity.y = y;
  }
    
  //minimal non-zero velocity
  if (velocity.x >= -0.01f && velocity.x <= 0.01f) velocity.x = 0;
  if (velocity.z >= -0.01f && velocity.z <= 0.01f) velocity.z = 0;
}

//==============================================================================
void Player::apply_deceleration(vec3 movement_direction, [[maybe_unused]] f32 delta_seconds)
{
  // If we are still -> return
  if (velocity.x == 0 && velocity.z == 0)
  {
    return;
  }

  vec3 reverseVelocity = vec3 (- velocity.x, 0, -velocity.z);
  reverseVelocity = normalize_or_zero(reverseVelocity);

  //apply deceleration if reverse key is pressed or if directional/axis key is not pressed
  if (movement_direction.x == 0 || signbit(movement_direction.x) != signbit(velocity.x))
  {
    velocity.x = velocity.x + (reverseVelocity.x * DECELERATION * delta_seconds);
  }

  if (movement_direction.z == 0 || signbit(movement_direction.z) != signbit(velocity.z))
  {
    velocity.z = velocity.z + (reverseVelocity.z * DECELERATION * delta_seconds);
  }

  //apply general deceleration
  velocity = velocity + (reverseVelocity * DECELERATION * delta_seconds);
}

//==============================================================================
void Player::update_gun_sway(f32 delta)
{
  const f32 move_fadein = CVars::gun_sway_move_fadein_time;
  const f32 air_fadein  = CVars::gun_sway_air_time;

  time_since_shoot      += delta;
  time_since_start      += delta;
  time_since_gun_change += delta;

  const bool is_moving = length(this->velocity) > 0.1f;
  const bool in_air    = !on_ground;

  smooth_towards(moving_time, is_moving ? move_fadein : 0.0f, delta);
  smooth_towards(air_time,    in_air    ? air_fadein  : 0.0f, delta);
}

//==============================================================================
void Player::damage(int damage)
{
  currentHealth -= damage;

  if (currentHealth < 0)
  {
    die();
  }
}

//==============================================================================
void Player::die()
{
  alive = false;
}

//==============================================================================
Camera* Player::get_camera()
{
  return &camera;
}

//==============================================================================
void Player::update(GameInputs curr_input, GameInputs prev_input, f32 delta)
{
  this->update_gun_sway(delta);
  this->calculate_wish_velocity(curr_input, delta);
  this->handle_weapon_change(curr_input, prev_input);
  this->handle_attack(curr_input, prev_input, delta);
  this->apply_velocity(delta);
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
  return view_height;
}

//==============================================================================
vec3& Player::get_velocity()
{
  return velocity;
}

//==============================================================================
WeaponType Player::get_equipped_weapon() const
{
  return current_weapon;
}

//==============================================================================
vec2 Player::get_gun_sway() const
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

  return vec2{max_x, max_y} * total_coeff * sway_amount + gun_change_offset;
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

}