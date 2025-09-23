// Project Nucledian Source File
#pragma once

#include <types.h>
#include <math/vector.h>
#include <math/matrix.h>

namespace nc
{

class Camera
{
public:
  static Camera* get();

  void update_transform(vec3 position, f32 yaw, f32 pitch);
  void update_transform(vec3 position, f32 yaw, f32 pitch, f32 y_offset);

  mat4 get_view()     const;
  vec3 get_forward()  const;
  vec3 get_position() const;

private:
  vec3 m_position = VEC3_ZERO;
  vec3 m_forward  = VEC3_ZERO;
  f32  m_yaw      = 0.0f;
  f32  m_pitch    = 0.0f;
};

}
