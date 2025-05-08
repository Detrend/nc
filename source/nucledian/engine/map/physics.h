// Project Nucledian Source File
#pragma once

#include <types.h>
#include <math/vector.h>

#include <engine/map/map_types.h>
#include <engine/entity/entity_types.h>

#include <vector>

namespace nc
{
struct MapSectors;
struct SectorMapping;
class  EntityRegistry;
}

namespace nc
{

struct RayHit
{
  // Returns true if we hit something
  operator bool() const;

  static RayHit no_hit();
  static RayHit build(f32 coeff, vec3 n);

  struct EntityHit
  {
    EntityID entity_id;
  };

  struct SectorHit
  {
    SectorID sector_id;
    WallID   wall_id;
  };

  enum HitType : u8
  {
    sector,
    entity,
  };

  union
  {
    EntityHit entity_hit;
    SectorHit secctor_hit;
  };

  f32     coeff = FLT_MAX;
  vec3    normal; // do .xz() for 2D raycast
  HitType type  = HitType::sector;
};


struct World
{
  const EntityRegistry& entities;
  const MapSectors&     map;
  const SectorMapping&  mapping;

  struct PortalSector
  {
    WallID   wall_id;
    SectorID sector_id;
  };
  using Portals = std::vector<PortalSector>;

  // Casts either a ray or circle and returns if it hit something.
  // Is expected to be used for collision detection.
  // Also returns hit coefficient, which is <= 1, but might be negative
  // if the expand parameter is non-zero and we are stuck in a wall. This
  // is actually a feature as it allows us to get un-stuck
  RayHit raycast2d_expanded
  (
    vec2           ray_start,                      // point to cast from
    vec2           ray_end,                        // cast to
    f32            expand,                         // 0 for ray, radius for circle
    EntityTypeMask ent_types = ~EntityTypeMask{0}, // which entity types should be hit and which ignored
    Portals*       out_portals = nullptr           // list of portals the ray went through
  ) const;

  RayHit raycast2d
  (
    vec2           ray_start,                      // point to cast from
    vec2           ray_end,                        // cast to
    EntityTypeMask ent_types = ~EntityTypeMask{0}, // which entity types should be hit and which ignored
    Portals*       out_portals = nullptr           // list of portals the ray went through
  ) const;

  RayHit raycast3d
  (
    vec3           ray_start,                      // point to cast from
    vec3           ray_end,                        // cast to
    EntityTypeMask ent_types = ~EntityTypeMask{0}, // which entities should be hit and which ignored
    Portals*       out_portals = nullptr           // list of portals the ray went through
  ) const;

  // Moves the "entity" with the given position, velocity and direction and checks
  // for collisions in the way and alters the values. If there is a nc portal in
  // the way then traverses it.
  void move_and_collide
  (
    vec3&          position, // the original position of the entity, will get changed
    vec3           velocity, // the velocity, does not change
    vec3&          forward,  // forward direction, changes after nc portal traversal
    f32            radius,   // radius of the entity
    f32            height,   // height of the entity
    f32            max_step, // max step height the entity can do
    EntityTypeMask colliders // types of entities to collide with
  );
};

}

