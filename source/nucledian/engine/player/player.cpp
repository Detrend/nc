#include <engine/player/player.h>
#include <engine/input/game_input.h>
#include <engine/input/input_system.h>
//#include <iostream>

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

    velocity = vec3(0, 0, 0);

    if (!alive)
    {
      return;
    }

    // movement keys
    if (input.player_inputs.keys & (1 << PlayerKeyInputs::forward))
    {
      velocity = add(velocity, vec3(0, 0, 1));
    }
    if (input.player_inputs.keys & (1 << PlayerKeyInputs::backward))
    {
      velocity = add(velocity, vec3(0, 0, -1));
    }
    if (input.player_inputs.keys & (1 << PlayerKeyInputs::left))
    {
      velocity = add(velocity, vec3(-1, 0, 0));
    }
    if (input.player_inputs.keys & (1 << PlayerKeyInputs::right))
    {
      velocity = add(velocity, vec3(1, 0, 0));
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
      velocity.x * right
      + velocity.y * vec3::Y
      + velocity.z * forward
    );

    position += movement_direction * 5.0 * 0.001;

    //std::cout << position.x << " " << position.y << " " << position.z << std::endl;

    camera.update_transform(position, angle_yaw, angle_pitch, 0.5f);
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