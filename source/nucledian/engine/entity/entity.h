// Project Nucledian Source File
#pragma once

#include <engine/entity/entity_types.h>

#include <types.h>       // f32
#include <math/vector.h> // vec3

namespace nc
{

struct SectorMapping;
struct Appearance;
struct Physics;

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

  Entity(vec3 position, f32 width, f32 height, bool collision);

  virtual bool did_collide(const Entity& collider);
  virtual void check_collision(const Entity& collider, vec3& velocity, f32 delta_seconds);

  f32 get_width() const;

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

  // =============================================
  //             Components
  // =============================================
  Appearance*       get_appearance();
  const Appearance* get_appearance() const;

  Physics*          get_physics();
  const Physics*    get_physics() const;

  // =============================================
  //              Utility
  // =============================================
  // Cast the base entity to other type of entity.
  // Returns Pointer to the other type or nullptr
  // if the conversion is not possible.
  template<typename T>
  T* as()
  {
    if (T::get_type_static() == this->get_type())
    {
      return static_cast<T*>(this);
    }
    return nullptr;
  }

protected:
  bool  collision;

private: friend class EntityRegistry;
  SectorMapping* m_mapping     = nullptr;
  EntityID       m_id_and_type = INVALID_ENTITY_ID;
  vec3           m_position;
  f32            m_radius2d;
  f32            m_height;
};

}
