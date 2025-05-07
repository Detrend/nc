// Project Nucledian Source File
#include <common.h>

#include <engine/entity/entity.h>
#include <engine/entity/sector_mapping.h>

namespace nc
{

//==============================================================================
Entity::Entity(vec3 position, f32 radius, f32 height)
: m_Position(position)
, m_Radius2D(radius)
, m_Height(height)
{

}

//==============================================================================
Entity::~Entity()
{
  // TODO: unregister entity from the grid
}

//==============================================================================
EntityID Entity::get_id() const
{
  return m_IdAndType;
}

//==============================================================================
EntityType Entity::get_type() const
{
  return m_IdAndType.type;
}

//==============================================================================
vec3 Entity::get_position() const
{
  return m_Position;
}

//==============================================================================
void Entity::set_position(vec3 np)
{
  if (m_Position != np)
  {
    m_Position = np;
    m_Mapping->on_entity_move(m_IdAndType, m_Position, m_Radius2D, m_Height);
  }
}

//==============================================================================
f32 Entity::get_radius() const
{
  return m_Radius2D;
}

//==============================================================================
void Entity::set_radius(f32 r)
{
  nc_assert(r >= 0.0f);
  if (m_Radius2D != r)
  {
    m_Radius2D = r;
    m_Mapping->on_entity_move(m_IdAndType, m_Position, m_Radius2D, m_Height);
  }
}

//==============================================================================
f32 Entity::get_height() const
{
  return m_Height;
}

//==============================================================================
void Entity::set_height(f32 nh)
{
  nc_assert(nh >= 0.0f);
  if (m_Height != nh)
  {
    m_Height = nh;
    m_Mapping->on_entity_move(m_IdAndType, m_Position, m_Radius2D, m_Height);
  }
}

//==============================================================================
void Entity::set_pos_rad_height(vec3 p, f32 r, f32 h)
{
  if (p != m_Position || r != m_Radius2D || h != m_Height)
  {
    m_Position = p;
    m_Radius2D = r;
    m_Height   = h;
    m_Mapping->on_entity_move(m_IdAndType, m_Position, m_Radius2D, m_Height);
  }
}

}

