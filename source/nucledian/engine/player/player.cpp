#include <engine/player/player.h>
#include <engine/input/game_input.h>

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

  PlayerSpecificInputs nc::Player::get_inputs()
  {
    currentInputs = lastInputs;
    currentInputs = PlayerSpecificInputs();

    SDL_SetRelativeMouseMode(SDL_TRUE);
    const u8* keyboard_state = SDL_GetKeyboardState(nullptr);



    if (keyboard_state[SDL_SCANCODE_W])
      currentInputs.keys = currentInputs.keys | (1 << PlayerKeyInputs::forward);
    if (keyboard_state[SDL_SCANCODE_S])
      currentInputs.keys = currentInputs.keys | (1 << PlayerKeyInputs::backward);
    if (keyboard_state[SDL_SCANCODE_A])
      currentInputs.keys = currentInputs.keys | (1 << PlayerKeyInputs::left);
    if (keyboard_state[SDL_SCANCODE_D])
      currentInputs.keys = currentInputs.keys | (1 << PlayerKeyInputs::right);

    int x, y;
    u8 rel_mouse_state = SDL_GetRelativeMouseState(&x, &y);


    f32 sensitivity = 0.05;
    angle_pitch -= y * sensitivity;
    angle_yaw += x * sensitivity;

    lastInputs = currentInputs;
    return currentInputs;
  }

  //==========================================================================

  void Player::load_inputs(PlayerSpecificInputs inputs)
  {
    lastInputs = currentInputs;
    currentInputs = inputs;
  }

  //==========================================================================

  void Player::update()
  {
    // INPUT HANDELING

    velocity = vec3(0, 0, 0);

    if (!alive) 
    {
      return;
    }

    // movement keys
    if (currentInputs.keys & (1 << PlayerKeyInputs::forward))
    {
      velocity = add(velocity, vec3(speed, 0, 0));
    }
    if (currentInputs.keys & (1 << PlayerKeyInputs::backward))
    {
      velocity = add(velocity, vec3(-speed, 0, 0));
    }
    if (currentInputs.keys & (1 << PlayerKeyInputs::left))
    {
      velocity = add(velocity, vec3(0, -speed, 0));
    }
    if (currentInputs.keys & (1 << PlayerKeyInputs::right))
    {
      velocity = add(velocity, vec3(0, speed, 0));
    }

    angle_pitch += currentInputs.analog[PlayerAnalogInputs::look_vertical];
    angle_yaw += currentInputs.analog[PlayerAnalogInputs::look_horizontal];

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


  //=================================================================================================

  void Player::handle_key_downs(SDL_Event& event)
  {
    // Here we use OR, as it is an addition
    switch (event.key.keysym.sym) {
    case SDLK_w:
      currentInputs.keys = currentInputs.keys | (1 << PlayerKeyInputs::forward);
      break;
    case SDLK_s:
      currentInputs.keys = currentInputs.keys | (1 << PlayerKeyInputs::backward);
      break;
    case SDLK_a:
      currentInputs.keys = currentInputs.keys | (1 << PlayerKeyInputs::left);
      break;
    case SDLK_d:
      currentInputs.keys = currentInputs.keys | (1 << PlayerKeyInputs::right);
      break;
    default:
      break;
    }
  }


  //===================================================================================

  void Player::handle_key_ups(SDL_Event& event)
  {
    // Here we use XOR, as we check only the previously pressed buttons
    switch (event.key.keysym.sym) {
    case SDLK_w:
      currentInputs.keys = currentInputs.keys ^ (1 << PlayerKeyInputs::forward);
      break;
    case SDLK_s:
      currentInputs.keys = currentInputs.keys ^ (1 << PlayerKeyInputs::backward);
      break;
    case SDLK_a:
      currentInputs.keys = currentInputs.keys ^ (1 << PlayerKeyInputs::left);
      break;
    case SDLK_d:
      currentInputs.keys = currentInputs.keys ^ (1 << PlayerKeyInputs::right);
      break;
    default:
      break;
    }
  }