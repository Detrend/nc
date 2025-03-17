#include <engine/player/player.h>
#include <engine/input/game_input.h>
#include <engine/input/input_system.h>
#include <iostream>

namespace nc
{
  Player::Player()
  {
    this->position = vec3(0.0f, 0.0f, 3.0f);
    currentHealth = maxHealth;

    width = 0.25f;
    height = 1.5f;
    collision = true;

    camera.update_transform(position, angle_yaw, angle_pitch, 0.5f);
  }

  //==========================================================================

  Player::Player(vec3 position)
  {
    this->position = position;
    currentHealth = maxHealth;

    width = 0.25f;
    height = 1.5f;
    collision = true;

    camera.update_transform(position, angle_yaw, angle_pitch, 0.5f);
  }

  //==========================================================================

  void Player::update(GameInputs input)
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
      inputVector = add(inputVector, vec3(0, 0, 1));
    }
    if (input.player_inputs.keys & (1 << PlayerKeyInputs::backward))
    {
      inputVector = add(inputVector, vec3(0, 0, -1));
    }
    if (input.player_inputs.keys & (1 << PlayerKeyInputs::left))
    {
      inputVector = add(inputVector, vec3(-1, 0, 0));
    }
    if (input.player_inputs.keys & (1 << PlayerKeyInputs::right))
    {
      inputVector = add(inputVector, vec3(1, 0, 0));
    }

    angle_pitch += input.player_inputs.analog[PlayerAnalogInputs::look_vertical];
    angle_yaw += input.player_inputs.analog[PlayerAnalogInputs::look_horizontal];

    angle_yaw = rem_euclid(angle_yaw, 2.0f * pi);
    angle_pitch = clamp(angle_pitch, -half_pi + 0.001f, half_pi - 0.001f);

    m_forward = angleAxis(angle_yaw, vec3::Y) * angleAxis(angle_pitch, vec3::X) * -vec3::Z;

    // APLICATION OF VELOCITY
    const vec3 forward = m_forward.with_y(0);
    const vec3 right = cross(forward, vec3::Y);
    const vec3 movement_direction = normalize_or_zero
    (
      inputVector.x * right
      + inputVector.y * vec3::Y
      + inputVector.z * forward
    );

    apply_deceleration(movement_direction);

    apply_acceleration(movement_direction);

    std::cout << velocity.x << " " << velocity.z << std::endl;
    position += velocity * 0.001f;

    //std::cout << position.x << " " << position.y << " " << position.z << std::endl;

    camera.update_transform(position, angle_yaw, angle_pitch, 0.5f);
  }

  void Player::apply_acceleration(const nc::vec3& movement_direction)
  {
    velocity += movement_direction * ACCELERATION * 0.001f;

    //minimal non-zero velocity
    if (velocity.x >= -0.005f && velocity.x <= 0.005f) velocity.x = 0;
    if (velocity.z >= -0.005f && velocity.z <= 0.005f) velocity.z = 0;

    //speed cap
    //if (sqrtf(velocity.x * velocity.x + velocity.z * velocity.z) > MAX_SPEED)
    if(length(vec2(velocity.x, velocity.z)) > MAX_SPEED)
    {
      velocity = normalize_or_zero(velocity) * MAX_SPEED;
    }
  }

  void Player::apply_deceleration(const nc::vec3& movement_direction)
  {
    // If we are still -> return
    if (velocity.x == 0 && velocity.z == 0)
    {
      return;
    }

    vec3 reverseVelocity = -velocity;
    reverseVelocity = normalize_or_zero(reverseVelocity);

    //apply deceleration if reverse key is pressed or if directional/axis key is not pressed
    if (movement_direction.x == 0 || signbit(movement_direction.x) != signbit(velocity.x))
    {
      velocity.x = velocity.x + (reverseVelocity.x * DECELERATION * 0.001f);
    }

    if (movement_direction.z == 0 || signbit(movement_direction.z) != signbit(velocity.z))
    {
      velocity.z = velocity.z + (reverseVelocity.z * DECELERATION * 0.001f);
    }

    //apply general deceleration
    velocity = velocity + (reverseVelocity * DECELERATION * 0.001f);
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
}