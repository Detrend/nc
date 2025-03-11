#pragma once

#include <SDL.h>
#include <types.h>
#include <vec.h>
#include <vector_maths.h>
#include <engine/input/game_input.h>
#include <engine/player/map_object.h>
#include <engine/graphics/debug_camera.h>

namespace nc
{

  class Player : MapObject
  {
  public:
    Player();
    Player(vec3 position);

    //PlayerSpecificInputs get_inputs();
    //void load_inputs(PlayerSpecificInputs inputs);
    void update(GameInputs input);
    void Damage(int damage);
    void Die();

    DebugCamera* get_camera();

  private:
    // THESE FUNCTIONS ARE OLD AND MIGHT BE TERMINATED
    //void handle_key_downs(SDL_Event& event);
    //void handle_key_ups(SDL_Event& event);

    vec3 position;
    f32 speed = 0.5;
    vec3 velocity;

    vec3 m_forward = vec3::ZERO;
    f32 angle_pitch = 0; //UP-DOWN
    f32 angle_yaw = 0; //LET-RIGHT

    /*PlayerSpecificInputs lastInputs;
    PlayerSpecificInputs currentInputs;*/

    int maxHealth = 100;
    int currentHealth;

    bool alive = true;

    DebugCamera camera;
  };

}