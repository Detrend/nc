#include <cvars.h>

#include <engine/player/player.h>
#include <engine/input/game_input.h>
#include <engine/input/input_system.h>
#include <iostream>

#include <engine/map/map_system.h>
#include <engine/map/physics.h>
#include <engine/player/thing_system.h>

#include <engine/core/engine.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <engine/entity/entity_type_definitions.h>
#include <imgui/imgui.h>

namespace nc
{
  //==========================================================================
  Player::Player(vec3 position)
    : Base(position, 0.25f, 1.5f, true)
  {
    currentHealth = maxHealth;
    camera.update_transform(position, angleYaw, anglePitch, viewHeight);
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

    anglePitch += input.player_inputs.analog[PlayerAnalogInputs::look_vertical];
    angleYaw += input.player_inputs.analog[PlayerAnalogInputs::look_horizontal];

    angleYaw = rem_euclid(angleYaw, 2.0f * PI);
    anglePitch = clamp(anglePitch, -HALF_PI + 0.001f, HALF_PI - 0.001f);

    if (CVars::lock_camera_pitch)
    {
      // set the pitch permanently to 0 if the camera is locked
      anglePitch = 0.0f;
    }

    m_forward = angleAxis(angleYaw, VEC3_Y) * angleAxis(anglePitch, VEC3_X) * -VEC3_Z;

    // APLICATION OF VELOCITY
    const vec3 forward = with_y(m_forward, 0);
    const vec3 right = cross(forward, VEC3_Y);
    const vec3 movement_direction = normalize_or_zero
    (
      inputVector.x * right
      + inputVector.y * VEC3_Y
      + inputVector.z * forward
    );

    //NC_MESSAGE("{} {}", velocity.x, velocity.z);
    //position += velocity * 0.001f;

    //NC_MESSAGE("{} {} {}", position.x, position.y, position.z);
    // GET FLOOR HEIGHT
    const auto& map = get_engine().get_map();
    f32 floor = 0;
    auto sector_id = map.get_sector_from_point(this->get_position().xz());
    if (sector_id != INVALID_SECTOR_ID)
    {
      const f32 sector_floor_y = map.sectors[sector_id].floor_height;
      floor = sector_floor_y;
    }

    // Did jump
    vec3 jumpForce = vec3(0, 0, 0);
    if (input.player_inputs.keys & (1 << PlayerKeyInputs::jump) && this->get_position().y > floor - 0.0001f && this->get_position().y < floor + 0.0001f)
    {
      jumpForce += vec3(0, 1, 0);
    }

    apply_deceleration(movement_direction, delta_seconds);

    apply_acceleration(movement_direction, delta_seconds);

    velocity += jumpForce * 3.0f;
    velocity.y -= GRAVITY * delta_seconds;

    camera.update_transform(this->get_position(), angleYaw, anglePitch, viewHeight);
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

  void Player::apply_velocity(f32 delta_seconds)
  {
    const auto& map = get_engine().get_map();
    const auto  lvl = ThingSystem::get().get_level();
    f32 floor = 0;
    auto sector_id = map.get_sector_from_point(this->get_position().xz());
    if (sector_id != INVALID_SECTOR_ID)
    {
      const f32 sector_floor_y = map.sectors[sector_id].floor_height;
      floor = sector_floor_y;
    }

    if (this->get_position().y + velocity.y * delta_seconds < floor)
    {
      //velocity.y = floor - this->get_position().y;
      velocity.y = 0;
    }

    vec3 velocity_per_frame = velocity * delta_seconds;
    vec3 position = this->get_position();
    lvl.move_and_collide(position, velocity_per_frame, m_forward, 0.25f, 0.25f, 0.25f, 0);
    this->set_position(position);

    // recompute the angleYaw after moving through a portal
    const auto forward2 = normalize(with_y(m_forward, 0.0f));
    angleYaw = rem_euclid(std::atan2f(forward2.z, -forward2.x) + HALF_PI, PI * 2);
  }

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

  void Player::Damage(int damage)
  {
    currentHealth -= damage;

    if (currentHealth < 0)
    {
      Die();
    }
  }

  void Player::Die()
  {
    alive = false;
  }

  DebugCamera* Player::get_camera()
  {
    return &camera;
  }

  vec3 Player::get_look_direction()
  {
    //looking direction
    return angleAxis(angleYaw, VEC3_Y) * angleAxis(anglePitch, VEC3_X) * -VEC3_Z;
  }

  f32 Player::get_view_height()
  {
    return viewHeight;
  }

  vec3& Player::get_velocity()
  {
    return velocity;
  }

  //==============================================================================
  EntityType Player::get_type_static()
  {
    return EntityTypes::player;
  }

}