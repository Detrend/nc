// Project Nucledian Source File
#pragma once

#include <types.h>
#include <nc_math.h>

namespace nc
{

class DebugCamera
{
public:
  DebugCamera();

  void handle_input(f32 delta_seconds);
  void update_transform(vec3 position, f32 yaw, f32 pitch);
  void update_transform(vec3 position, f32 yaw, f32 pitch, f32 y_offset);
  mat4 get_view() const;
  vec3 get_forward()  const;
  vec3 get_position() const;

private:
  void handle_movement(f32 delta_seconds);
  void handle_rotation();

private:
  // Camera's movement speed.
  inline static constexpr f32 SPEED       = 5.0f;
  // Camera's rotation sensitivity.
  inline static constexpr f32 SENSITIVITY = 0.002f;

private:
  bool  m_first_frame = true;
  vec3  m_position    = vec3(0.0f, 0.0f, 3.0f);
  // Camera's facing direction. Precomputed every frame by handle_rotation(f32).
  vec3  m_forward     = glm::zero<vec3>();
  f32   m_yaw         = 0.0f;
  f32   m_pitch       = 0.0f;
};

}