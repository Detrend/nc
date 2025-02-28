#include <engine/player/player.h>
#include <engine/input/game_input.h>

Player::Player(nc::vec3 position)
{
  this->position = position;
}

//==========================================================================

void Player::update()
{
  // INPUT HANDELING
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
      angle_yaw += event.motion.xrel * 0.05; // MAGIC VALUES NEED TO BE REPLACED LATER ON
      angle_pitch += event.motion.xrel * 0.05;
      break;
    default:
      break;
    }
  }

  // APLICATION OF VELOCITY
  position.x += velocity.x * sinf(angle_yaw);
  position.y += velocity.y *= cosf(angle_yaw);
}


//=================================================================================================

void Player::handle_key_downs(SDL_Event& event)
{
  switch (event.key.keysym.sym) {
  case SDLK_w:
    velocity = nc::add(velocity, nc::vec3(speed, 0, 0));
    break;
  case SDLK_s:
    velocity = nc::add(velocity, nc::vec3(-speed, 0, 0));
    break;
  case SDLK_a:
    velocity = nc::add(velocity, nc::vec3(0, -speed, 0));
    break;
  case SDLK_d:
    velocity = nc::add(velocity, nc::vec3(0, speed, 0));
    break;
  default:
    break;
  }
}


//===================================================================================

void Player::handle_key_ups(SDL_Event& event)
{
  switch (event.key.keysym.sym) {
  case SDLK_w:
    velocity = nc::add(velocity, nc::vec3(-speed, 0, 0));
    break;
  case SDLK_s:
    velocity = nc::add(velocity, nc::vec3(speed, 0, 0));
    break;
  case SDLK_a:
    velocity = nc::add(velocity, nc::vec3(0, speed, 0));
    break;
  case SDLK_d:
    velocity = nc::add(velocity, nc::vec3(0, -speed, 0));
    break;
  default:
    break;
  }
}
