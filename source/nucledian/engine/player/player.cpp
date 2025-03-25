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

    camera.update_transform(position, angleYaw, anglePitch, viewHeight);
  }

  //==========================================================================

  Player::Player(vec3 position)
  {
    this->position = position;
    currentHealth = maxHealth;

    width = 0.25f;
    height = 1.5f;
    collision = true;

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

    anglePitch += input.player_inputs.analog[PlayerAnalogInputs::look_vertical];
    angleYaw += input.player_inputs.analog[PlayerAnalogInputs::look_horizontal];

    angleYaw = rem_euclid(angleYaw, 2.0f * pi);
    anglePitch = clamp(anglePitch, -half_pi + 0.001f, half_pi - 0.001f);

    m_forward = angleAxis(angleYaw, vec3::Y) * angleAxis(anglePitch, vec3::X) * -vec3::Z;

    // APLICATION OF VELOCITY
    const vec3 forward = m_forward.with_y(0);
    const vec3 right = cross(forward, vec3::Y);
    const vec3 movement_direction = normalize_or_zero
    (
      inputVector.x * right
      + inputVector.y * vec3::Y
      + inputVector.z * forward
    );

    apply_deceleration(movement_direction, delta_seconds);

    apply_acceleration(movement_direction, delta_seconds);

    //std::cout << velocity.x << " " << velocity.z << std::endl;
    //position += velocity * 0.001f;

    //std::cout << position.x << " " << position.y << " " << position.z << std::endl;

    camera.update_transform(position, angleYaw, anglePitch, viewHeight);
  }

  void Player::check_collision(MapObject collider)
  {
    position += velocity;

    if (!did_collide(collider))
    {
      position += -velocity;
      return;
    }

    vec3 target_dist = collider.get_position() - position;

    vec3 intersect_union = vec3(get_width(), 0, get_width()) - (target_dist - vec3(collider.get_width(), 0, collider.get_width()));

    position += -velocity;

    vec3 new_velocity = velocity - intersect_union;
    vec3 mult = vec3(velocity.x / (1 - new_velocity.x), 0, velocity.z / (1 - new_velocity.z));

    if (mult.x < 0.05f) mult.x = 0;
    if (mult.z < 0.05f) mult.z = 0;

    target_dist = collider.get_position() - position;
    target_dist.x = abs(target_dist.x);
    target_dist.z = abs(target_dist.z);

    if (target_dist.x >= width + collider.get_width())
    {
      mult.z = 1;
    }

    if (target_dist.z >= width + collider.get_width())
    {
      mult.x = 1;
    }

    velocity.x = velocity.x * mult.x;
    velocity.z = velocity.z * mult.z;
    
  }

  void Player::apply_velocity()
  {
    //std::cout << velocity.x << "|" << velocity.z << std::endl;
    position += velocity;
  }

  void Player::apply_acceleration(const nc::vec3& movement_direction, f32 delta_seconds)
  {
    velocity += movement_direction * ACCELERATION * delta_seconds;

    //speed cap
    //if (sqrtf(velocity.x * velocity.x + velocity.z * velocity.z) > MAX_SPEED)
    if(length(vec2(velocity.x, velocity.z)) > MAX_SPEED * delta_seconds)
    {
      velocity = normalize_or_zero(velocity) * MAX_SPEED * delta_seconds;
    }
    
    //minimal non-zero velocity
    if (velocity.x >= -0.01f * delta_seconds && velocity.x <= 0.01f * delta_seconds) velocity.x = 0;
    if (velocity.z >= -0.01f * delta_seconds && velocity.z <= 0.01f * delta_seconds) velocity.z = 0;
  }

  void Player::apply_deceleration(const nc::vec3& movement_direction, f32 delta_seconds)
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

  vec3 Player::get_position()
  {
    return position;
  }
}