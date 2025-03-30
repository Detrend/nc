// Project Nucledian Source File
#include <engine/graphics/debug_camera.h>

#include <SDL2/include/SDL.h>

#include <numbers>
#include <bit>

namespace nc
{

//==============================================================================
DebugCamera::DebugCamera()
{
  //SDL_SetRelativeMouseMode(SDL_TRUE);
}

//==============================================================================
void DebugCamera::handle_input(float delta_seconds)
{
  this->handle_movement(delta_seconds);
  this->handle_rotation();

  m_first_frame = false;
}

//==============================================================================

void DebugCamera::update_transform(vec3 position, f32 yaw, f32 pitch)
{
  m_position = position;
  m_yaw = yaw;
  m_pitch = pitch;

  // taken from handle rotation
  m_yaw = rem_euclid(m_yaw, 2.0f * pi);
  m_pitch = clamp(m_pitch, -half_pi + 0.001f, half_pi - 0.001f);

  m_forward = angleAxis(m_yaw, VEC3_Y) * angleAxis(m_pitch, VEC3_X) * -VEC3_Z;
}

//==============================================================================

void DebugCamera::update_transform(vec3 position, f32 yaw, f32 pitch, f32 y_offset)
{
  m_position = position;
  m_position.y += y_offset;
  m_yaw = yaw;
  m_pitch = pitch;

  // taken from handle rotation
  m_yaw = rem_euclid(m_yaw, 2.0f * pi);
  m_pitch = clamp(m_pitch, -half_pi + 0.001f, half_pi - 0.001f);

  m_forward = angleAxis(m_yaw, VEC3_Y) * angleAxis(m_pitch, VEC3_X) * -VEC3_Z;
}

//==============================================================================
mat4 DebugCamera::get_view() const
{
  return lookAt(m_position, m_position + m_forward, VEC3_Y);
}

//==============================================================================
vec3 DebugCamera::get_forward() const
{
  return m_forward;
}

//==============================================================================
vec3 DebugCamera::get_position() const
{
  return m_position;
}

//==============================================================================
void DebugCamera::handle_movement(float delta_seconds)
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
void DebugCamera::handle_rotation()
{
  int delta_x, delta_y;
  SDL_GetRelativeMouseState(&delta_x, &delta_y);
  const vec2 mouse_pos_delta(static_cast<f32>(delta_x), static_cast<f32>(delta_y));

  /*if (!m_first_frame && length2(mouse_pos_delta) < 2.0f)
  {
    return;
  }*/

  m_yaw -= mouse_pos_delta.x * SENSITIVITY;
  m_pitch -= mouse_pos_delta.y * SENSITIVITY;

  m_yaw = rem_euclid(m_yaw, 2.0f * pi);
  m_pitch = clamp(m_pitch, -half_pi + 0.001f, half_pi - 0.001f);

  m_forward = angleAxis(m_yaw, VEC3_Y) * angleAxis(m_pitch, VEC3_X) * -VEC3_Z;
}

}
