// Project Nucledian Source File
#include <engine/graphics/camera.h>

#include <engine/player/player.h>
#include <engine/player/thing_system.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <SDL2/include/SDL.h>

#include <numbers>
#include <bit>

namespace nc
{
//==============================================================================
Camera* Camera::get()
{
  return ThingSystem::get().get_player()->get_camera();
}

//==============================================================================
void Camera::handle_input(float delta_seconds)
{
  this->handle_movement(delta_seconds);
  this->handle_rotation();

  m_first_frame = false;
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

//==============================================================================
void Camera::handle_movement(float delta_seconds)
{
  const u8* keyboard_state = SDL_GetKeyboardState(nullptr);

  vec3 input_direction = glm::zero<vec3>();
  if (keyboard_state[SDL_SCANCODE_W])
    input_direction.z += 1.0f;
  if (keyboard_state[SDL_SCANCODE_A])
    input_direction.x -= 1.0f;
  if (keyboard_state[SDL_SCANCODE_S])
    input_direction.z -= 1.0f;
  if (keyboard_state[SDL_SCANCODE_D])
    input_direction.x += 1.0f;
  if (keyboard_state[SDL_SCANCODE_SPACE])
    input_direction.y += 1.0f;
  if (keyboard_state[SDL_SCANCODE_LCTRL])
    input_direction.y -= 1.0f;

  const vec3 forward = with_y(m_forward, 0);
  const vec3 right = cross(forward, VEC3_Y);
  const vec3 movement_direction = normalize_or_zero
  (
      input_direction.x * right
    + input_direction.y * VEC3_Y
    + input_direction.z * forward
  );
    
  m_position += movement_direction * SPEED * delta_seconds;
}

//==============================================================================
void Camera::handle_rotation()
{
  int delta_x, delta_y;
  SDL_GetRelativeMouseState(&delta_x, &delta_y);
  const vec2 mouse_pos_delta(static_cast<f32>(delta_x), static_cast<f32>(delta_y));

  m_yaw -= mouse_pos_delta.x * SENSITIVITY;
  m_pitch -= mouse_pos_delta.y * SENSITIVITY;

  m_yaw = rem_euclid(m_yaw, 2.0f * PI);
  m_pitch = clamp(m_pitch, -HALF_PI + 0.001f, HALF_PI - 0.001f);

  m_forward = angleAxis(m_yaw, VEC3_Y) * angleAxis(m_pitch, VEC3_X) * -VEC3_Z;
}

}
