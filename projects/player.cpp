#include "player.h"

Player::Player(nc::vec3 position)
{
  this->position = position;
}

void Player::update()
{
  SDL_Event event;;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_KEYDOWN:
      handle_key_downs(event);
      break;
    case SDL_KEYUP:
      handle_key_ups(event);
      break;
    default:
      break;
    }

  }
}

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
