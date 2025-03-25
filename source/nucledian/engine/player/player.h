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

  class Player : public MapObject
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

    vec3 get_position();

  private:
    void apply_acceleration(const nc::vec3& movement_direction);
    void apply_deceleration(const nc::vec3& movement_direction);

    //vec3 position;
    vec3 velocity; // forward/back - left/right velocity
    f32 viewHeight = 0.5f;

    f32 MAX_SPEED = 4.5f;
    f32 ACCELERATION = 30.0f;
    f32 DECELERATION = 15.0f;

    vec3 m_forward = vec3::ZERO;
    f32 anglePitch = 0; //UP-DOWN
    f32 angleYaw = 0; //LET-RIGHT

    /*PlayerSpecificInputs lastInputs;
    PlayerSpecificInputs currentInputs;*/

    int maxHealth = 100;
    int currentHealth;

    bool alive = true;

    DebugCamera camera;
  };

}