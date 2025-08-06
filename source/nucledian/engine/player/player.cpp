#include <cvars.h>

#include <engine/player/player.h>
#include <engine/input/game_input.h>
#include <engine/input/input_system.h>
#include <iostream>

#include <engine/map/map_system.h>
#include <engine/map/physics.h>
#include <engine/player/thing_system.h>
#include <engine/entity/entity_system.h>

#include <game/weapons.h>

#include <engine/core/engine.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <engine/entity/entity_type_definitions.h>
#include <imgui/imgui.h>

namespace nc
{
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

  void Player::get_wish_velocity(GameInputs input, f32 delta_seconds)
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
void Player::handle_inputs(GameInputs input, GameInputs /*prev_input*/)
{
  for (WeaponType i = 0; i < WeaponTypes::count; ++i)
  {
    PlayerKeyFlags weapon_flag = 1 << (PlayerKeyInputs::weapon_0 + i);

    const bool owns_weapon  = this->has_weapon(i);
    const bool wants_weapon = input.player_inputs.keys & weapon_flag;

    if (owns_weapon && wants_weapon)
    {
      current_weapon = i;

      // Accept only the first one if multiple keys are pressed
      break; 
    }
  }
}

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
    PhysLevel       lvl = ThingSystem::get().get_level();
    EntityRegistry& ecs = ThingSystem::get().get_entities();

    vec3 position = this->get_position();

    EntityTypeMask all_colliders = PhysLevel::COLLIDE_ALL;

    EntityTypeMask ok_colliders  = all_colliders & ~EntityTypeFlags::player;
    EntityTypeMask report_only   = EntityTypeFlags::pickup;

    PhysLevel::CharacterCollisions collisions;
    lvl.move_character
    (
      position, velocity, &m_forward, delta_seconds,
      0.25f, 1.0f, 0.25f, ok_colliders, report_only, &collisions
    );
    this->set_position(position);

    // Set on ground if touching floor
    this->on_ground = !collisions.floors.empty();

    // Process pickups
    for (EntityID pickup_id : collisions.report_entities)
    {
      if (PickUp* pickup = ecs.get_entity<PickUp>(pickup_id))
      {
        pickup->on_pickup(*this);
      }
      ecs.destroy_entity(pickup_id);
    }

    // recompute the angleYaw after moving through a portal
    const auto forward2 = normalize(with_y(m_forward, 0.0f));
    angle_yaw = rem_euclid(std::atan2f(forward2.z, -forward2.x) + HALF_PI, PI * 2);
  }

  //==============================================================================

  void Player::apply_acceleration(const nc::vec3& movement_direction, [[maybe_unused]] f32 delta_seconds)
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

  void Player::apply_deceleration(const nc::vec3& movement_direction, [[maybe_unused]] f32 delta_seconds)
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

  Camera* Player::get_camera()
  {
    return &camera;
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