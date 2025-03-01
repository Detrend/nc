#include <engine/player/player.h>
#include <engine/input/game_input.h>

namespace nc
{
  Player::Player(vec3 position)
  {
    this->position = position;
  }

  //==========================================================================

  PlayerSpecificInputs nc::Player::get_inputs()
  {
    currentInputs = lastInputs;

    SDL_Event event;;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_KEYDOWN:
        handle_key_downs(event);
        break;
      case SDL_KEYUP:
        handle_key_ups(event);
        break;
      case SDL_MOUSEMOTION:
        currentInputs.analog[PlayerAnalogInputs::look_horizontal] = event.motion.xrel * 0.05f; // MAGIC VALUES NEED TO BE REPLACED LATER ON
        currentInputs.analog[PlayerAnalogInputs::look_vertical] = event.motion.xrel * 0.05f;
        break;
      default:
        break;
      }
    }

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