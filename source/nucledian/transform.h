#pragma once

#include <temp_math.h>

namespace nc
{

// TODO: Transform is too big and probably an overkill, there is a chance there would be need to replace it by other
// solution when thing system is implemented.

/**
 * @brief Represents a 3D transformation, including position, rotation, and scale.
 *
 * The Transform class provides methods to manipulate and retrieve the position, rotation,
 * and scale of an object in 3D space. It also caches the transformation matrix for efficient
 * computation when changes occur.
 */
class Transform
{
public:
  Transform(const vec3& position = vec3::ZERO, const vec3& scale = vec3::ONE, const vec3& rotation = vec3::ZERO);

  static Transform from_position(const vec3& position);
  static Transform from_scale(const vec3& scale);
  static Transform from_rotation(const vec3& rotation);

  Transform with_position(const vec3& position) const;
  Transform with_scale(const vec3& scale) const;
  Transform with_rotation(const vec3& rotation) const;

  vec3& position();
  vec3& scale();
  // Gets a reference to the rotation vector (Euler angles).
  vec3& rotation();

  const vec3& get_position() const;
  const vec3& get_scale() const;
  const vec3& get_rotation() const;

  f32& rotation_x();
  f32& rotation_y();
  f32& rotation_z();

  f32 get_rotation_x() const;
  f32 get_rotation_y() const;
  f32 get_rotation_z() const;

  /**
   * @brief Gets the transformation matrix.
   *
   * The transformation matrix is cached and only recomputed if the position, scale, or rotation
   * has changed since the last call.
   */
  const mat4& get_matrix();

private:
  // Indicates whether the transformation matrix needs to be recomputed.
  bool m_dirty = true;

  vec3 m_position = vec3::ZERO;
  vec3 m_scale = vec3::ONE;
  // The rotation of the transform in Euler angles.
  vec3 m_rotation = vec3::ZERO;

  // The cached transformation matrix.
  mat4 m_matrix;
};

}
