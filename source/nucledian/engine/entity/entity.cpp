// Project Nucledian Source File
#include <common.h>

#include <engine/entity/entity.h>
#include <engine/entity/entity_type_definitions.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_system.h>
#include <engine/core/engine.h>
#include <engine/map/map_system.h>

#include <math/lingebra.h>
#include <engine/game/game_system.h>

#include <engine/player/player.h>
#include <engine/enemies/enemy.h>
#include <engine/graphics/entities/lights.h>

#include <game/projectile.h>
#include <game/item.h>
#include <game/particle.h>

#include <engine/graphics/entities/prop.h>

#include <set>
#include <map>

namespace nc
{

//==============================================================================
Entity::Entity(vec3 position, f32 radius, f32 height)
: m_position(position)
, m_radius2d(radius)
, m_height(height)
{

}

//==============================================================================
Entity::Entity(vec3 position, f32 radius)
: Entity(position, radius, radius)
{

}

//==============================================================================
Entity::~Entity()
{

}

//==============================================================================
EntityID Entity::get_id() const
{
  return m_id_and_type;
}

//==============================================================================
EntityType Entity::get_type() const
{
  return m_id_and_type.type;
}

//==============================================================================
vec3 Entity::get_position() const
{
  return m_position;
}

//==============================================================================
void Entity::set_position(vec3 np)
{
  if (m_position != np)
  {
    m_position = np;
    m_registry->on_entity_move_internal(m_id_and_type, m_position, m_radius2d, m_height);
  }
}

//==============================================================================
f32 Entity::get_radius() const
{
  return m_radius2d;
}

//==============================================================================
void Entity::set_radius(f32 r)
{
  nc_assert(r >= 0.0f);
  if (m_radius2d != r)
  {
    m_radius2d = r;
    m_registry->on_entity_move_internal(m_id_and_type, m_position, m_radius2d, m_height);
  }
}

//==============================================================================
f32 Entity::get_height() const
{
  return m_height;
}

//==============================================================================
void Entity::set_height(f32 nh)
{
  nc_assert(nh >= 0.0f);
  if (m_height != nh)
  {
    m_height = nh;
    m_registry->on_entity_move_internal(m_id_and_type, m_position, m_radius2d, m_height);
  }
}

//==============================================================================
void Entity::set_pos_rad_height(vec3 p, f32 r, f32 h)
{
  if (p != m_position || r != m_radius2d || h != m_height)
  {
    m_position = p;
    m_radius2d = r;
    m_height   = h;
    m_registry->on_entity_move_internal(m_id_and_type, m_position, m_radius2d, m_height);
  }
}

//==============================================================================
Appearance* Entity::get_appearance()
{
  switch (this->get_type())
  {
    case EntityTypes::enemy:      return &this->as<Enemy>()->get_appearance();
    case EntityTypes::projectile: return this->as<Projectile>()->get_appearance();
    case EntityTypes::pickup:     return &this->as<PickUp>()->get_appearance();
    case EntityTypes::prop:       return &this->as<Prop>()->get_appearance();
    case EntityTypes::particle:   return this->as<Particle>()->get_appearance();
    default:                      return nullptr;
  }
}

//==============================================================================
const Appearance* Entity::get_appearance() const
{
  return const_cast<Entity*>(this)->get_appearance();
}

//==============================================================================
Physics* Entity::get_physics()
{
  return nullptr;
}

//==============================================================================
const Physics* Entity::get_physics() const
{
  return const_cast<Entity*>(this)->get_physics();
}

//==============================================================================
SectorSnapType Entity::get_snap_type() const
{
  switch (this->get_type())
  {
    case EntityTypes::point_light: return this->as<PointLight>()->snap;
    case EntityTypes::pickup: return SectorSnapTypes::floor;
    case EntityTypes::prop:   return this->as<Prop>()->get_snap_type();
    default:                  return SectorSnapTypes::free;
  }
}

//==============================================================================
f32 Entity::get_snap_offset() const
{
  if (this->get_type() == EntityTypes::point_light)
  {
    return this->as<PointLight>()->snap_offset;
  }

  if (this->get_snap_type() == SectorSnapTypes::ceiling)
  {
    return this->get_height();
  }

  return 0.0f;
}

}
