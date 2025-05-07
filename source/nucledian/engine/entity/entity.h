// Project Nucledian Source File
#pragma once

#include <engine/entity/entity_types.h>

#include <types.h>       // f32
#include <math/vector.h> // vec3

namespace nc
{
struct SectorMapping;
}

namespace nc
{

class Entity
{
public:
  Entity(vec3 position, f32 radius, f32 height);
  virtual ~Entity();

  // Deleted copy assignment and construction to not accidentally
  // copy the entity
  Entity(const Entity&)            = delete;
  Entity& operator=(const Entity&) = delete;

  // Each entity type has to contain a public static constexpr
  // member variable containing it's type! Like this:
  // static EntityType get_type_static();

  // =============================================
  // Non virtual interface - same for each entity
  // =============================================
  EntityID   get_id()   const;
  EntityType get_type() const;

  vec3       get_position() const;
  void       set_position(vec3 np);

  f32        get_radius() const;
  void       set_radius(f32 r);

  f32        get_height() const;
  void       set_height(f32 nh);

  void       set_pos_rad_height(vec3 p, f32 r, f32 h);

private: friend class EntityRegistry;
  SectorMapping* m_Mapping   = nullptr;
  EntityID       m_IdAndType = INVALID_ENTITY_ID;
  vec3           m_Position;
  f32            m_Radius2D;
  f32            m_Height;
};

}
