// Project Nucledian Source File
#include <engine/graphics/debug_camera.h>

#include <SDL2/include/SDL.h>

#include <numbers>
#include <bit>

namespace nc
{

//==============================================================================
void DebugCamera::handle_input(float delta_seconds)
{
  this->handle_movement(delta_seconds);
  this->handle_rotation(delta_seconds);
}

//==============================================================================
mat4 DebugCamera::get_view() const
{
  return look_at(m_position, m_position + m_forward, vec3::Y);
}

//==============================================================================
void DebugCamera::handle_movement(float delta_seconds)
{
  const u8* keyboard_state = SDL_GetKeyboardState(nullptr);

  vec3 input_direction = vec3::ZERO;
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

  const vec3 forward = m_forward.with_y(0);
  const vec3 right = cross(forward, vec3::Y);
  const vec3 movement_direction = normalize_or_zero
  (
      input_direction.x * right
    + input_direction.y * vec3::Y
    + input_direction.z * forward
  );
    
  m_position += movement_direction * SPEED * delta_seconds;
}

//==============================================================================
void DebugCamera::handle_rotation(float delta_seconds)
{
  int delta_x, delta_y;
  SDL_GetRelativeMouseState(&delta_x, &delta_y);
  const vec2 mouse_pos_delta(static_cast<f32>(delta_x), static_cast<f32>(delta_y));

  m_yaw -= mouse_pos_delta.x * SENSITIVITY * delta_seconds;
  m_pitch -= mouse_pos_delta.y * SENSITIVITY * delta_seconds;

  m_yaw = rem_euclid(m_yaw, 2.0f * pi);
  m_pitch = clamp(m_pitch, -half_pi + 0.001f, half_pi - 0.001f);

  m_forward = angleAxis(m_yaw, vec3::Y) * angleAxis(m_pitch, vec3::X) * -vec3::Z;
}

}
