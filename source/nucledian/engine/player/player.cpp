#include <engine/player/player.h>
#include <engine/input/game_input.h>
#include <engine/input/input_system.h>

namespace nc
{
  Player::Player(vec3 position)
  {
    this->position = position;
    currentHealth = maxHealth;

    width = 0.25f;
    height = 1.5f;
    collision = true;
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
      velocity = add(velocity, vec3(speed, 0, 0));
    }
    if (input.player_inputs.keys & (1 << PlayerKeyInputs::backward))
    {
      velocity = add(velocity, vec3(-speed, 0, 0));
    }
    if (input.player_inputs.keys & (1 << PlayerKeyInputs::left))
    {
      velocity = add(velocity, vec3(0, -speed, 0));
    }
    if (input.player_inputs.keys & (1 << PlayerKeyInputs::right))
    {
      velocity = add(velocity, vec3(0, speed, 0));
    }

    angle_pitch += input.player_inputs.analog[PlayerAnalogInputs::look_vertical];
    angle_yaw += input.player_inputs.analog[PlayerAnalogInputs::look_horizontal];

    // APLICATION OF VELOCITY
    position.x += velocity.x * sinf(angle_yaw);
    position.y += velocity.y * cosf(angle_yaw);
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
}