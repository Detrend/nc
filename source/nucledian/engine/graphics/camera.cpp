// Project Nucledian Source File
#include <engine/graphics/camera.h>

#include <engine/player/player.h>
#include <engine/player/game_system.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <numbers>
#include <bit>

namespace nc
{

//==============================================================================
Camera* Camera::get()
{
  return GameSystem::get().get_player()->get_camera();
}

//==============================================================================
void Camera::update_transform(vec3 position, f32 yaw, f32 pitch)
{
  m_position = position;
  m_yaw = yaw;
  m_pitch = pitch;

  // taken from handle rotation
  m_yaw = rem_euclid(m_yaw, 2.0f * PI);
  m_pitch = clamp(m_pitch, -HALF_PI + 0.001f, HALF_PI - 0.001f);

  m_forward = angleAxis(m_yaw, VEC3_Y) * angleAxis(m_pitch, VEC3_X) * -VEC3_Z;
}

//==============================================================================
void Camera::update_transform(vec3 position, f32 yaw, f32 pitch, f32 y_offset)
{
  m_position = position;
  m_position.y += y_offset;
  m_yaw = yaw;
  m_pitch = pitch;

  // taken from handle rotation
  m_yaw = rem_euclid(m_yaw, 2.0f * PI);
  m_pitch = clamp(m_pitch, -HALF_PI + 0.001f, HALF_PI - 0.001f);

  m_forward = angleAxis(m_yaw, VEC3_Y) * angleAxis(m_pitch, VEC3_X) * -VEC3_Z;
}

//==============================================================================
mat4 Camera::get_view() const
{
  return lookAt(m_position, m_position + m_forward, VEC3_Y);
}

//==============================================================================
vec3 Camera::get_forward() const
{
  return m_forward;
}

//==============================================================================
vec3 Camera::get_position() const
{
  return m_position;
}

}
