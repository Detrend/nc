#include <transform.h>

namespace nc
{

Transform::Transform(const vec3& position, const vec3& scale, const vec3& rotation)
  : m_position(position), m_scale(scale), m_rotation(rotation) {}

//==========================================================================
Transform Transform::from_position(const vec3& position)
{
  return Transform(position);
}

//==========================================================================
Transform Transform::from_scale(const vec3& scale)
{
  return Transform(vec3::ZERO, scale);
}

//==========================================================================
Transform Transform::from_rotation(const vec3& rotation)
{
  return Transform(vec3::ZERO, vec3::ONE, rotation);
}

//==========================================================================
Transform Transform::with_position(const vec3& position) const
{
  return Transform(position, m_scale, m_rotation);
}

//==========================================================================
Transform Transform::with_scale(const vec3& scale) const
{
  return Transform(m_position, scale, m_rotation);
}

//==========================================================================
Transform Transform::with_rotation(const vec3& rotation) const
{
  return Transform(m_position, m_scale, rotation);
}

//==========================================================================
vec3& Transform::position()
{
  m_dirty = true;
  return m_position;
}

//==========================================================================
vec3& Transform::scale()
{
  m_dirty = true;
  return m_scale;
}

//==========================================================================
vec3& Transform::rotation()
{
  m_dirty = true;
  return m_rotation;
}

//==========================================================================
const vec3& Transform::get_position() const
{
  return m_position;
}

//==========================================================================
const vec3& Transform::get_scale() const
{
  return m_scale;
}

//==========================================================================
const vec3& Transform::get_rotation() const
{
  return m_rotation;
}

//==========================================================================
f32& Transform::rotation_x()
{
  m_dirty = true;
  return m_rotation.x;
}

//==========================================================================
f32& Transform::rotation_y()
{
  m_dirty = true;
  return m_rotation.y;
}

//==========================================================================
f32& Transform::rotation_z()
{
  m_dirty = true;
  return m_rotation.z;
}

//==========================================================================
f32 Transform::get_rotation_x() const
{
  return m_rotation.x;
}

//==========================================================================
f32 Transform::get_rotation_y() const
{
  return m_rotation.y;
}

//==========================================================================
f32 Transform::get_rotation_z() const
{
  return m_rotation.z;
}

//==========================================================================
const mat4& Transform::get_matrix()
{
  if (m_dirty)
  {
    m_matrix = translate(mat4(1.0f), m_position)
      * eulerAngleXYZ(m_rotation.x, m_rotation.y, m_rotation.z)
      * nc::scale(mat4(1.0f), m_scale);
    m_dirty = false;
  }

  return m_matrix;
}

}
