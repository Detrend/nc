// Project Nucledian Source File
#pragma once

#include <types.h>
#include <temp_math.h>

namespace nc
{

class DebugCamera
{
public:
  void handle_input(float delta_seconds);
  mat4 get_view() const;

private:
  void handle_movement(float delta_seconds);
  void handle_rotation(float delta_seconds);

private:
  // Camera's movement speed.
  inline static constexpr float SPEED       = 5.0f;
  // Camera's rotation sensitivity.
  inline static constexpr float SENSITIVITY = 2.0f;

private:
  vec3  m_position = vec3(0.0f, 0.0f, 3.0f);
  vec3  m_forward  = vec3::ZERO;
  f32   m_yaw      = 0.0f;
  f32   m_pitch    = 0.0f;
};

}