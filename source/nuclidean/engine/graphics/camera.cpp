// Project Nuclidean Source File
#include <engine/graphics/camera.h>

#include <engine/player/player.h>
#include <engine/game/game_helpers.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <numbers>
#include <bit>

namespace nc
{

//==============================================================================
Camera* Camera::get()
{
  if (Player* player = GameHelpers::get().get_player())
  {
    return player->get_camera();
  }

  return nullptr;
}

//==============================================================================
void Camera::update_transform(vec3 position, f32 yaw, f32 pitch, f32 roll)
{
  update_transform(position, yaw, pitch, roll, 0.0f);
}

//==============================================================================
void Camera::update_transform(vec3 position, f32 yaw, f32 pitch, f32 roll, f32 y_offset)
{
  m_position = position;
  m_position.y += y_offset;
  // taken from handle rotation
  yaw   = rem_euclid(yaw, 2.0f * PI);
  pitch = clamp(pitch, -HALF_PI + 0.001f, HALF_PI - 0.001f);

  m_up      = angleAxis(yaw, VEC3_Y) * angleAxis(roll,  VEC3_Z) *  VEC3_Y;
  m_forward = angleAxis(yaw, VEC3_Y) * angleAxis(pitch, VEC3_X) * -VEC3_Z;
}

//==============================================================================
mat4 Camera::get_view() const
{
  return lookAt(m_position, m_position + m_forward, m_up);
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
