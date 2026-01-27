// Project Nucledian Source File
#pragma once

#include <types.h>
#include <math/vector.h>
#include <math/matrix.h>

#include <engine/map/map_types.h>
#include <engine/entity/entity_types.h>

#include <stack_vector.h>

#include <vector>
#include <functional>
#include <compare>    // <=>

namespace nc
{
struct MapSectors;
struct SectorMapping;
class  EntityRegistry;
}

namespace nc
{

// Physics component. Not sure if we are ever gonna use it
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
struct CollisionHit
{
  // Returns true if we hit something. Can be used in condition
  // to check if the RayHit contains some result.
  operator bool() const;

  // Three way comparison that compares two hits based on their
  // distance in the manner "shorter hit < longer hit"
  f32 operator<=>(const CollisionHit& other) const;

  struct SectorHit;
  struct EntityHit;

  // === Construction helpers ===

  // Constructs empty hit
  static CollisionHit no_hit();

  // Constructs sector hit
  static CollisionHit build(f32 coeff, vec3 n, SectorHit sh);

  // Constructs entity hit
  static CollisionHit build(f32 coeff, vec3 n, EntityHit eh);

  // Basics checking
  bool is_hit()        const;
  bool is_sector_hit() const;
  bool is_floor_hit()  const;
  bool is_ceil_hit()   const;
  bool is_wall_hit()   const;
  bool is_entity_hit() const;

  // Internals
  enum HitType : u8
  {
    sector = 0,
    entity = 1,
  };

  enum SectorHitType : u8
  {
    // first 2 bits reserved for type
    wall      =  0b001,
    floor     =  0b010,
    ceil      =  0b100,

    // third bit for nuclidean/non-nuclidean
    nuclidean = 0b1000,

    nuclidean_wall = wall | nuclidean,
    ceil_or_floor  = ceil | floor
  };

  struct EntityHit
  {
    EntityID entity_id;
  };

  struct SectorHit
  {
    SectorID      sector_id;       // always valid
    WallID        wall_id;         // only valid if type == wall
    u8            wall_segment_id; // only valid if type == wall, unused for now
    SectorHitType type;
  };

  union Hit
  {
    EntityHit entity; // valid only if type == entity
    SectorHit sector; // valid only if type == sector
  };

  f32     coeff = FLT_MAX; // coefficient to recalculate hit pos from
  vec3    normal;          // do .xz() for 2D raycast
  Hit     hit;
  HitType type  = HitType::sector;
};


// Physical level representation. Level compiles all physics-related game
// systems into one interface through which the physics can be queried.
struct PhysLevel
{
  const EntityRegistry& entities;
  const MapSectors&     map;
  const SectorMapping&  mapping;

  static constexpr EntityTypeMask COLLIDE_ALL  = static_cast<EntityTypeMask>(-1);
  static constexpr EntityTypeMask COLLIDE_NONE = EntityTypeMask{0};

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
  CollisionHit circle_cast_2d
  (
    vec2           ray_start,                 // point to cast from
    vec2           ray_end,                   // cast to
    f32            expand,                    // 0 for ray, radius for circle
    EntityTypeMask ent_types   = COLLIDE_ALL, // which entity types should be hit and which ignored
    Portals*       out_portals = nullptr      // list of portals the ray went through
  ) const;

  CollisionHit ray_cast_2d
  (
    vec2           ray_start,                 // point to cast from
    vec2           ray_end,                   // cast to
    EntityTypeMask ent_types   = COLLIDE_ALL, // which entity types should be hit and which ignored
    Portals*       out_portals = nullptr      // list of portals the ray went through
  ) const;

	// Casts a 3D ray and reports the first thing it hits
  CollisionHit ray_cast_3d
  (
    vec3           ray_start,                 // point to cast from
    vec3           ray_end,                   // cast to
    EntityTypeMask ent_types   = COLLIDE_ALL, // which entities should be hit and which ignored
    Portals*       out_portals = nullptr      // list of portals the ray went through
  ) const;

	// Casts a cylinder shape between two points and checks if it intersects something
	// in the world.
  CollisionHit cylinder_cast_3d
  (
    vec3           ray_start,                 // point to cast from
    vec3           ray_end,                   // point to cast to
    f32            expand,                    // radius of the cylinder
    f32            height,                    // height of the cylinder
    EntityTypeMask ent_types   = COLLIDE_ALL, // which entity types should be hit and which ignored
    Portals*       out_portals = nullptr      // list of portals the ray went through
  ) const;

  // For recording simulation hits
  using CollisionListener = std::function<void(const CollisionHit& /*hit*/)>;

  struct CharacterCollisions
  {
    static constexpr u64 STACK_COLLISIONS = 8;
    struct WallCollision
    {
      WallID   wall;
      SectorID sector;
    };

    template<typename T>
    using SmallVector = StackVector<T, STACK_COLLISIONS>;

    SmallVector<EntityID>      report_entities;
    SmallVector<EntityID>      entities;
    SmallVector<WallCollision> walls;
    SmallVector<SectorID>      floors;
    SmallVector<SectorID>      ceilings;
  };

  // Moves the "entity" with the given position, velocity and direction and checks
  // for collisions in the way and alters the values. If there is a nc portal in
  // the way then traverses it.
  void move_character
  (
    vec3&             position,          // the original position of the entity, will get changed
    vec3&             velocity,          // the velocity
    mat4&             transform_mod,     // transform after portal transition
    f32               delta_time,        // frame time
    f32               radius,            // radius of the entity
    f32               height,            // height of the entity
    f32               step,              // max step height the entity can do
    EntityTypeMask    colliders,         // types of entities to collide with
    EntityTypeMask    report_only,       // types of entities to only report the collision, but do not collide with them
    CharacterCollisions* collisions_opt = nullptr
  ) const;

	// Moves a non-character physics object in the level. The object bounces around
	// and collides with other objects. The collision detection is continuous and
  // therefore can be used for simulation of projectiles.
  // All objects have a cylinder-like collider with radius and a height
  void move_particle
  (
    vec3&             position,  // starting position, changed by the function
    vec3&             velocity,  // starting velocity, changed by the function
    mat4&             transform, // transform, changes after passing through portals
    f32&              delta_time,// frametime in seconds
    f32               radius,    // radius of the cylinder
    f32               height,    // height of the cylinder
    f32               neg_height,// -y offset of the cylinder start
    f32               bounce,    // bounce factor, 1 = normal bounce
    EntityTypeMask    colliders, // what entities to collide with
    CollisionListener listener = nullptr // reaction to collisions
  ) const;

  // Computes a path. Points which were reached through non-euclidean portals
  // are RELATIVE to the sector where start_pos is in.
  // After a NPC traverses through a non-euclidean portal these points have to
  // be transformed by the portal transform.
  std::vector<vec3> calc_path_relative
  (
    vec3  start_pos,                 // Starting position
    vec3  end_pos,                   // Target position
    f32   radius,                    // Radius of the entity
    f32   height,                    // Max height of the ceiling
    f32   step_up,                   // Max step height, positive or 0
    f32   step_down,                 // Max drop down height, positive or 0
    bool  do_smoothing     = true,   // Should the path be smoothed out?
    mat4* nc_transform_opt = nullptr // Nucledian transformation of the portals
  ) const;

  // Calculates the loudness of the sound in 3D world.
  // Returns a value in [0, 1] interval.
  f32 calc_3d_sound_volume
  (
    vec3 camera_pos,
    vec3 sound_pos,
    f32  sound_distance
  ) const;

  // Spreads into sectors near the point and returns their IDs
  void floodfill_nearby_sectors
  (
    vec3                       point,
    f32                        distance,
    f32                        sector_height_threshold,
    StackVector<SectorID, 32>& sectors_out
  ) const;
};

}
