// Project Nucledian Source File
#pragma once

#include <types.h>
#include <math/vector.h>

#include <engine/map/map_types.h>
#include <engine/entity/entity_types.h>

#include <vector>
#include <functional>

namespace nc
{
struct MapSectors;
struct SectorMapping;
class  EntityRegistry;
class  Entity;
}

namespace nc
{

// Physics component
struct Physics
{
  vec3           velocity;
  EntityTypeMask collide_with;  // types of entities we collide with
  EntityTypeMask report_only;   // do not collide, but report the collisions
  f32            max_step_height = 0.0f;
  f32            bounce          = 0.0f;
};

// Return value of the raycast. Can be a sector hit or
// entity hit.
struct RayHit
{
  // Returns true if we hit something. Can be used in condition
  // to check if the RayHit contains some result.
  operator bool() const;

  // Construction helpers
  static RayHit no_hit();
  static RayHit build(f32 coeff, vec3 n);

  struct EntityHit
  {
    EntityID entity_id;
  };

  struct SectorHit
  {
    SectorID sector_id;       // always valid
    WallID   wall_id;         // only valid if type == wall
    u8       wall_segment_id; // only valid if type == wall
  };

  enum HitType : u8
  {
    sector = 1,
    entity = 1 << 5,

    sector_wall  = sector | 1 << 2,
    sector_floor = sector | 1 << 3,
    sector_ceil  = sector | 1 << 4,
  };

  union
  {
    EntityHit entity_hit; // valid only if type == entity
    SectorHit sector_hit; // valid only if type == sector
  };

  f32     coeff = FLT_MAX; // coefficient to recalculate hit pos from
  vec3    normal;          // do .xz() for 2D raycast
  HitType type  = HitType::sector_wall;
};


// Physical level representation. Level compiles all physics-related game
// systems into one interface through which the physics can be queried.
struct PhysLevel
{
  const EntityRegistry& entities;
  const MapSectors&     map;
  const SectorMapping&  mapping;

  static constexpr EntityTypeMask COLLIDE_EVERYTHING = static_cast<EntityTypeMask>(-1);
  static constexpr EntityTypeMask COLLIDE_NOTHING    = EntityTypeMask{0};

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

  enum CollisionReaction : u8
  {
    continue_simulation, // continues the simulation of movement
    stop_simulation,     // stops simulating and exits the function
  };
  using CollisionListener = std::function<CollisionReaction(const RayHit& hit)>;
  // Moves the "entity" with the given position, velocity and direction and checks
  // for collisions in the way and alters the values. If there is a nc portal in
  // the way then traverses it.
  void move_and_collide
  (
    vec3&             position,          // the original position of the entity, will get changed
    vec3&             velocity,          // the velocity
    vec3*             opt_forward,       // forward direction, changes after nc portal traversal
    f32               delta_time,        // frame time
    f32               radius,            // radius of the entity
    f32               height,            // height of the entity
    f32               max_step,          // max step height the entity can do
    EntityTypeMask    colliders,         // types of entities to collide with
    EntityTypeMask    report_only,       // do not collide with these, but only report collisions
    f32               bounce   = 0.0f,   // 
    CollisionListener listener = nullptr // reaction to collisions
  ) const;

  // Move helper for entities with physics component.
  void move_and_collide
  (
    Entity&           ent,
    vec3*             forward,
    f32               frame_time,
    CollisionListener listener = nullptr
  );
};

}

