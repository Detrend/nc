#pragma once

#include <SDL.h>
#include <types.h>
#include <vec.h>
#include <vector_maths.h>

class Player
{
public:
  Player(nc::vec3 position);

  void update();

private:
  void handle_key_downs(SDL_Event& event);
  void handle_key_ups(SDL_Event& event);

  nc::vec3 position;
  nc::f32 speed = 0.5;
  nc::vec3 velocity;

  nc::f32 angle_pitch = 0; //UP-DOWN
  nc::f32 angle_yaw = 0; //LET-RIGHT
};

