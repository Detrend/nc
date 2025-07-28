// Project Nucledian Source File
#include <engine/map/physics.h>

#include <engine/map/map_system.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/sector_mapping.h>

#include <math/lingebra.h>

#include <intersect.h>

#include <set>
#include <utility>  // std::pair
#include <type_traits>

namespace nc::phys_helpers
{

//==============================================================================
template<typename T>
static aabb2 calc_stationary_bbox(T ray_from, f32 expand)
{
  const vec2 offset = vec2{expand};

  if constexpr (std::is_same_v<T, vec2>)
  {
    return aabb2{ray_from + offset, ray_from - offset};
  }
  else
  {
    static_assert(std::is_same_v<T, vec3>);
    return aabb2{ray_from.xz() + offset, ray_from.xz() - offset};
  }
}

//==============================================================================
template<typename T>
static f32 calc_dist_to_bbox(T pt, const aabb3& bbox)
{
  if constexpr (std::is_same_v<T, vec2>)
  {
    aabb2 bbox2;
    bbox2.min = bbox.min.xz();
    bbox2.max = bbox.max.xz();
    return dist::point_aabb(pt, bbox2);
  }
  else
  {
    static_assert(std::is_same_v<T, vec3>);
    return dist::point_aabb(pt, bbox);
  }
}

//==============================================================================
static vec2 vector_to_2d(vec2 v)
{
  return v;
}

//==============================================================================
static vec2 vector_to_2d(vec3 v)
{
  return v.xz();
}

//==============================================================================
// bool(TVec ray_from, TVec ray_to, f32 expand, const WallData& w1, const SectorData& sector, const WallData& w2, f32& c, f32& n)
template
<
  typename TVec,
  typename WallHitLambda,
  typename SectorHitLambda,
  typename EntityHitLambda
>
CollisionHit raycast_generic
(
  const PhysLevel&    world,
  TVec                ray_from,
  TVec                ray_to,
  f32                 expand,
  EntityTypeMask      ent_types,
  PhysLevel::Portals* out_portals,
  WallID              ignore_portal,
  WallHitLambda       wall_intersect,
  SectorHitLambda     sector_intersect,
  EntityHitLambda     entity_intersect
)
{
  using SectorHitType = CollisionHit::SectorHitType;
  using HitType       = CollisionHit::HitType;

  nc_assert(expand >= 0.0f, "Radius can not be negative!");

  std::set<SectorID> overlap_sectors;

  if (vector_to_2d(ray_from) == vector_to_2d(ray_to))
  {
    // stationary
    aabb2 bbox = calc_stationary_bbox(ray_from, expand);
    world.map.sector_grid.query_aabb(bbox, [&](aabb2, SectorID sid)
    {
      overlap_sectors.insert(sid);
      return false; // continue iteration
    });
  }
  else
  {
    // moving
    world.map.sector_grid.query_ray(ray_from, ray_to, expand, [&](aabb2, SectorID sid)
    {
      overlap_sectors.insert(sid);
      return false; // continue iteration
    });
  }

  CollisionHit best_hit = CollisionHit::no_hit();
  u32  num_hits    = 1;
  vec3 sum_normals = VEC3_ZERO;

  auto add_possible_hit = [&](const CollisionHit& hit)
  {
    if (hit < best_hit)
    {
      // this hit is just better
      best_hit    = hit;
      num_hits    = 1;
      sum_normals = hit.normal;
    }
    // This causes retardation.. Not sure why
    else if (hit == best_hit && hit.hit.sector.type == best_hit.hit.sector.type)
    {
      // they are the same, average them out
      num_hits    += 1;
      sum_normals += hit.normal;
    }
  };

  // Iterate all sectors that the ray might possibly intersect
  for (const SectorID sector_id : overlap_sectors)
  {
    nc_assert(world.map.is_valid_sector_id(sector_id));
    const SectorData& sector     = world.map.sectors[sector_id];
    const auto        begin_wall = sector.int_data.first_wall;
    const auto        end_wall   = sector.int_data.last_wall;
    const auto&       bbox3      = world.map.sector_bboxes[sector_id];

    // Calculate approximate distance to the sector. If the closest
    // point of the sector bbox is further than the closest raycasted
    // point so far then we can skip this sector.
    const f32 distance_to_closest_pt  = calc_dist_to_bbox(ray_from, bbox3);
    const f32 closest_hit_dist_so_far = length(ray_to - ray_from) * best_hit.coeff;
    if (best_hit && distance_to_closest_pt > closest_hit_dist_so_far)
    {
      // We can safely ignore this sector as it is too far away
      continue;
    }

    f32  c = FLT_MAX;
    vec3 n = vec3{0};

    // Check floor and ceiling intersections
    const bool sector_hit = sector_intersect
    (
      world.map, ray_from, ray_to, expand, sector_id, c, n
    );

    if (sector_hit)
    {
      // Floor/ceiling can't be a nuclidean portal
      add_possible_hit(CollisionHit::build(c, n, CollisionHit::SectorHit
      {
        .sector_id       = sector_id,
        .wall_id         = INVALID_WALL_ID,
        .wall_segment_id = 0,
        .type            = SectorHitType::floor, // TODO: add option for ceiling
      }));
    }

    // Check wall intersections
    // TODO[perf]: In theory, we do not need to check all the walls.
    // Once we hit one all other walls further away can be ignored.
    for (WallID wall_id = begin_wall; wall_id < end_wall; ++wall_id)
    {
      const WallID    next_wall_id = map_helpers::next_wall(world.map, sector_id, wall_id);
      const WallData& wall_data    = world.map.walls[wall_id];

      const auto portal_type  = wall_data.get_portal_type();
      const bool is_nc_portal = portal_type == PortalType::non_euclidean;

      if (wall_id == ignore_portal)
      {
        // ignore this portal, because we got here either from a previous
        // iteration or this is a normal portal.
        nc_assert(is_nc_portal);
        continue;
      }

      bool is_nc_hit = false;
      c = FLT_MAX;
      n = vec3{0};

      const bool does_intersect = wall_intersect
      (
        world.map, ray_from, ray_to, expand, wall_id,
        next_wall_id, sector_id, c, n, is_nc_hit
      );

      if (does_intersect)
      {
        SectorHitType hit_type = is_nc_hit ? SectorHitType::nuclidean_wall : SectorHitType::wall;
        add_possible_hit(CollisionHit::build(c, n, CollisionHit::SectorHit
        {
          .sector_id       = sector_id,
          .wall_id         = wall_id,
          .wall_segment_id = 0,
          .type            = hit_type,
        }));
      }
    }

    // Check entity intersections
    for (auto entity_id : world.mapping.sectors_to_entities.entities[sector_id])
    {
      if (!(entity_id.type & ent_types))
      {
        // skip this entity, we do not check it
        continue;
      }

      // Broad phase - check the distance to bbox first
      const Entity* entity = world.entities.get_entity(entity_id);
      nc_assert(entity);
      f32  r = entity->get_radius();
      f32  h = entity->get_height();
      vec3 p = entity->get_position();
      aabb3 bbox
      {
        p - vec3{r, 0, r} - vec3{expand},
        p + vec3{r, h, r} + vec3{expand}
      };

      f32 closest_pt_on_bbox = calc_dist_to_bbox(ray_from, bbox);
      f32 closest_pt_so_far  = length(ray_from - ray_to) * best_hit.coeff;
      if (best_hit && closest_pt_so_far < closest_pt_on_bbox)
      {
        // The point we hit is already closer, no need to check the entity
        continue;
      }

      const bool does_intersect = entity_intersect
      (
        world, ray_from, ray_to, expand, *entity, c, n
      );

      if (does_intersect)
      {
        // TODO
        add_possible_hit(CollisionHit::build(c, n, CollisionHit::EntityHit
        {
          .entity_id = entity_id
        }));
      }
    }
  }

  if (best_hit)
  {
    best_hit.normal = normalize(sum_normals);
  }

  // Recurse into nuclidean portal if we have to
  if (best_hit && best_hit.type == HitType::sector
    && best_hit.hit.sector.type & SectorHitType::nuclidean)
  {
    nc_assert(best_hit.coeff >= 0.0f && best_hit.coeff <= 1.0f, "NC portal behind us?");

    // Add this portal to the list if we collect them
    if (out_portals)
    {
      out_portals->push_back(PhysLevel::PortalSector
      {
        .wall_id   = best_hit.hit.sector.wall_id,
        .sector_id = best_hit.hit.sector.sector_id,
      });
    }

    const auto hit_pt = ray_from + (ray_to - ray_from) * best_hit.coeff;
    const auto proj = world.map.calc_portal_to_portal_projection
    (
      best_hit.hit.sector.sector_id,
      best_hit.hit.sector.wall_id
    );

    // Calculate new from and to points for the next raycast
    TVec new_from;
    TVec new_to;
    if constexpr (std::is_same_v<TVec, vec3>)
    {
      new_from = (proj * vec4{hit_pt, 1}).xyz();
      new_to   = (proj * vec4{ray_to, 1}).xyz();
    }
    else
    {
      new_from = (proj * vec4{hit_pt.x, 0, hit_pt.y, 1}).xz();
      new_to   = (proj * vec4{ray_to.x, 0, ray_to.y, 1}).xz();
    }

    // We need to ignore the portal wall we came throught in the next run,
    // because we could enter an infinite recursion, as we collide with
    // the wall
    WallID wall_to_ignore = map_helpers::get_nc_opposing_wall
    (
      world.map, best_hit.hit.sector.sector_id, best_hit.hit.sector.wall_id
    );

    // And cast the ray recursively
    const CollisionHit hit = raycast_generic<TVec>
    (
      world, new_from, new_to, expand, ent_types, out_portals, wall_to_ignore,
      wall_intersect, sector_intersect, entity_intersect
    );

    if (hit)
    {
      // out normal has to be projected to our space (not the other portal space)
      const auto transform_inv = inverse(proj);
      const auto reproject_norm = (transform_inv * vec4{hit.normal, 0.0f}).xyz();

      // recalculate the out_coeff and out_normal
      f32  final_coeff  = (1.0f - best_hit.coeff) * hit.coeff;
      vec3 final_normal = reproject_norm;

      best_hit = hit;
      best_hit.coeff  = final_coeff;
      best_hit.normal = final_normal;
    }
    else
    {
      // we did not hit anything behind the portal
      best_hit = CollisionHit::no_hit();
    }
  }

  // Return true if we hit something
  return best_hit;
}

}

namespace nc
{ 
  
//==============================================================================
CollisionHit::operator bool() const
{
  return coeff != FLT_MAX;
}

//==============================================================================
f32 CollisionHit::operator<=>(const CollisionHit& other) const
{
  return this->coeff - other.coeff;
}

//==============================================================================
CollisionHit CollisionHit::no_hit()
{
  return CollisionHit
  {
    .coeff = FLT_MAX
  };
}

//==============================================================================
CollisionHit CollisionHit::build(f32 c, vec3 n, SectorHit sh)
{
  return CollisionHit
  {
    .coeff  = c,
    .normal = n,
    .hit    = Hit{.sector = sh},
    .type   = HitType::sector,
  };
}

//==============================================================================
CollisionHit CollisionHit::build(f32 c, vec3 n, EntityHit eh)
{
  return CollisionHit
  {
    .coeff  = c,
    .normal = n,
    .hit    = eh,
    .type   = HitType::entity,
  };
}

//==============================================================================
static bool intersect_wall_2d
(
  const MapSectors& map,
  vec2              ray_from,
  vec2              ray_to,
  f32               expand,
  WallID            w1id,
  WallID            w2id,
  SectorID          /*sid*/,
  f32&              out_c,
  vec3&             out_n,
  bool&             nc_hit
)
{
  out_n  = VEC3_ZERO;
  out_c  = FLT_MAX;
  nc_hit = false;

  if (ray_from == ray_to)
  {
    // No need to check wall intersection
    return false;
  }

  const WallData& w1 = map.walls[w1id];
  const WallData& w2 = map.walls[w2id];

  PortType portal_type = w1.get_portal_type();
  if (portal_type == PortalType::classic)
  {
    // ignore normal portals in 2D
    return false;
  }

  // Normal collision check first
  vec2 n   = vec2{0};
  f32  c   = FLT_MAX;
  bool hit = false;

  f32 exp = portal_type == PortalType::non_euclidean ? 0.0f : expand;
  if (exp)
  {
    // Expanded wall check
    hit = collide::ray_exp_wall
    (
      ray_from, ray_to, w1.pos, w2.pos, expand, n, c
    );
  }
  else
  {
    // Non-expanded wall check
    hit = collide::ray_wall
    (
      ray_from, ray_to, w1.pos, w2.pos, n, c
    );
  }

  if (hit)
  {
    out_c  = c;
    out_n  = vec3{n.x, 0.0f, n.y};
    nc_hit = portal_type == PortalType::non_euclidean;
  }

  return hit;
}

//==============================================================================
static bool intersect_sector_2d
(
  [[maybe_unused]] const MapSectors& map,
  [[maybe_unused]] vec2              from,
  [[maybe_unused]] vec2              to,
  [[maybe_unused]] f32               expand,
  [[maybe_unused]] SectorID          sid,
  [[maybe_unused]] f32&              out_c,
  [[maybe_unused]] vec3&             out_n
)
{
  return false;
}

//==============================================================================
template<typename TVec>
static bool intersect_entity_empty
(
  [[maybe_unused]] const PhysLevel& level,
  [[maybe_unused]] TVec             from,
  [[maybe_unused]] TVec             to,
  [[maybe_unused]] f32              expand,
  [[maybe_unused]] const Entity&    entity,
  [[maybe_unused]] f32&             c_out,
  [[maybe_unused]] vec3&            n_out
)
{
  return false;
}

//==============================================================================
CollisionHit PhysLevel::circle_cast_2d
(
  vec2            from,
  vec2            to,
  f32             expand,
  EntityTypeMask  ent_types,
  PhysLevel::Portals* out_portals
) const
{
  return phys_helpers::raycast_generic<vec2>
  (
    *this, from, to, expand, ent_types, out_portals, INVALID_WALL_ID,
    &intersect_wall_2d, &intersect_sector_2d, &intersect_entity_empty<vec2>
  );
}

//==============================================================================
CollisionHit PhysLevel::ray_cast_2d
(
  vec2            from,
  vec2            to,
  EntityTypeMask  ent_types,
  PhysLevel::Portals* out_portals
) const
{
  return this->circle_cast_2d(from, to, 0.0f, ent_types, out_portals);
}

//==============================================================================
static bool intersect_wall_3d
(
  const MapSectors& map,
  vec3              ray_from,
  vec3              ray_to,
  f32               /*expand*/,
  WallID            w1id,
  WallID            w2id,
  SectorID          sid,
  f32&              out_c,
  vec3&             out_n,
  bool&             nc_hit
)
{
  if (ray_from == ray_to)
  {
    return false;
  }

  const WallData&   w1 = map.walls[w1id];
  const WallData&   w2 = map.walls[w2id];
  const SectorData& sd = map.sectors[sid];

  const bool hit = intersect::ray_wall_3d
  (
    ray_from, ray_to, w1.pos, w2.pos, sd.floor_height, sd.ceil_height, out_c
  );

  nc_hit = w1.get_portal_type() == PortalType::non_euclidean;

  nc_assert(w1.pos != w2.pos);
  auto n2 = flipped(normalize(w2.pos - w1.pos));
  out_n = vec3{n2.x, 0.0f, n2.y};

  return hit;
}

//==============================================================================
static bool intersect_sector_3d_height
(
  const MapSectors& map,
  vec3              from,
  vec3              to,
  f32               expand,
  f32               y_floor_add,
  f32               y_ceil_sub,
  SectorID          sid,
  f32&              out_c,
  vec3&             out_n
)
{
  nc_assert(y_floor_add >= 0.0f && y_ceil_sub >= 0.0f);

  out_n = VEC3_ZERO;
  out_c = FLT_MAX;

  if (from == to)
  {
    return false;
  }

  const SectorData& sector = map.sectors[sid];
  const f32  fy  = sector.floor_height + y_floor_add;
  const f32  cy  = sector.ceil_height  - y_ceil_sub;
  const vec3 dir = to - from;

  const f32 min_c = -(y_floor_add + y_ceil_sub) / length(to - from);
  const f32 max_c = 1.0f;

  for (auto[floor_y, normal] : {std::pair{fy, UP_DIR}, std::pair{cy, -UP_DIR}})
  {
    if (dot(dir, normal) > 0.0f)
    {
      // The ray points in direction out, ignore.
      // This handles also the cases when we stand on a floor and try to jump - 
      // in such cases we still collide with the ground (coeff == 0.0f), even though
      // we want to move away from it.
      // This handles such exceptions.
      continue;
    }

    if (f32 out; collide::ray_plane_xz(from, to, floor_y, out))
    {
      if (out >= min_c && out <= max_c && out < out_c)
      {
        out_c = out;
        out_n = normal;
      }
    }
  }

  if (out_c == FLT_MAX)
  {
    return false;
  }

  if (vec2 pt = from.xz() + (to - from).xz() * out_c; expand == 0.0f)
  {
    return map.is_point_in_sector(pt, sid);
  }
  else
  {
    return map.distance_from_sector_2d(pt, sid) <= expand;
  }
}

//==============================================================================
static bool intersect_sector_3d
(
  const MapSectors& map,
  vec3              from,
  vec3              to,
  f32               expand,
  SectorID          sid,
  f32&              out_c,
  vec3&             out_n
)
{
  return intersect_sector_3d_height
  (
    map, from, to, expand, 0, 0, sid, out_c, out_n
  );
}

//==============================================================================
CollisionHit PhysLevel::ray_cast_3d
(
  vec3           ray_start,
  vec3           ray_end,
  EntityTypeMask ent_types,
  Portals*       out_portals
)
const
{
  return phys_helpers::raycast_generic<vec3>
  (
    *this, ray_start, ray_end, 0.0f, ent_types, out_portals, INVALID_WALL_ID,
    &intersect_wall_3d, &intersect_sector_3d, &intersect_entity_empty<vec3>
  );
}

//==============================================================================
enum class StairWalkSettings
{
  Enabled,
  Disabled,
};

template<StairWalkSettings STAIR_WALK>
struct CylCastWallIntersector
{
  f32 height      = 0.0f;
  f32 step_height = 0.0f; // only if stair walk enabled

  bool operator()
  (
    const MapSectors& map,
    vec3              ray_from,
    vec3              ray_to,
    f32               expand,
    WallID            wid1,
    WallID            wid2,
    SectorID          sid,
    f32&              out_c,
    vec3&             out_n,
    bool&             out_nc_hit
  )
  const
  {
    return this->check
    (
      map, ray_from, ray_to, expand, wid1, wid2, sid, out_c, out_n, out_nc_hit
    );
  }

  bool check
  (
    const MapSectors& map,
    vec3              ray_from,
    vec3              ray_to,
    f32               expand,
    WallID            wid1,
    WallID            wid2,
    SectorID          sid,
    f32&              out_c,
    vec3&             out_n,
    bool&             out_nc_hit
  )
  const
  {
    nc_assert(expand > 0.0f);

    // Reset to default state EVERY TIME
    out_n      = VEC3_ZERO;
    out_c      = FLT_MAX;
    out_nc_hit = false;

    // No need to check anything if we do not move horizontally
    if (ray_from.xz() == ray_to.xz())
    {
      return false;
    }

    const WallData&   w1 = map.walls[wid1];
    const WallData&   w2 = map.walls[wid2];
    const SectorData& sc = map.sectors[sid];

    const f32 floor_y = sc.floor_height;
    const f32 ceil_y  = sc.ceil_height;

    // Now split the cases - it is either a full wall, or only a partial one.
    // In both cases we can do 2D raycast and then transform it into the
    // 3D world.
    auto portal_type = w1.get_portal_type();

    // Do the collision checking either way
    vec2 out_n_2d;
    f32  out_c_2d;
    bool hit = collide::ray_exp_wall
    (
      ray_from.xz(), ray_to.xz(), w1.pos, w2.pos,
      expand, out_n_2d, out_c_2d
    );

    // Fill out these just in case
    out_n      = vec3{out_n_2d.x, 0, out_n_2d.y};
    out_c      = out_c_2d;
    out_nc_hit = false;

    // We hit the normal wall
    if (hit && portal_type == PortalType::none)
    {
      // No need to check anything else, this is a guaranteed hit
      return true;
    }

    // We might still have hit the window, lets check
    if (hit)
    {
      // Partial - have to check if there is a free window
      nc_assert(map.is_valid_sector_id(w1.portal_sector_id));
      const SectorData& neighbor = map.sectors[w1.portal_sector_id];

      // Calculate the size of the window we can potentially fit into
      f32 window_from_y = std::max(neighbor.floor_height, floor_y);
      f32 window_to_y   = std::min(neighbor.ceil_height,  ceil_y) - height;

      if constexpr (STAIR_WALK == StairWalkSettings::Enabled)
      {
        // Enable us to make a step up
        window_from_y -= step_height;
      }

      // Check if we can pass through the free window to the other sector
      f32 hit_y = ray_from.y + (ray_to.y - ray_from.y) * out_c;
      if (hit_y < window_from_y || hit_y > window_to_y)
      {
        // We hit the solid wall, not the window.. exit
        return true;
      }
    }

    // If we got here then we either did not hit anything or we hit a
    // window..
    if (portal_type == PortalType::non_euclidean)
    {
      // Check for nc hit as well.. We traverse the NC portal only if our
      // center of mass goes through it. Therefore, no expansion is happening
      // here.
      bool nc_hit = collide::ray_wall
      (
        ray_from.xz(), ray_to.xz(), w1.pos, w2.pos, out_n_2d, out_c_2d
      );

      // We hit NC portal
      if (nc_hit)
      {
        out_c      = out_c_2d;
        out_n      = vec3{out_n_2d.x, 0, out_n_2d.y};
        out_nc_hit = true;
        return true;
      }
    }

    // Did not hit anything
    out_c      = FLT_MAX;
    out_n      = VEC3_ZERO;
    out_nc_hit = false;
    return false;
  };
};

//==============================================================================
// Does only the 2D check for now
struct CylCastEntityIntersector
{
  bool operator()
  (
    const PhysLevel& lvl,
    vec3             ray_from,
    vec3             ray_to,
    f32              expand,
    const Entity&    entity,
    f32&             out_c,
    vec3&            out_n
  )
  const
  {
    return this->check(lvl, ray_from, ray_to, expand, entity, out_c, out_n);
  }

  bool check
  (
    const PhysLevel& /*lvl*/,
    vec3             ray_from,
    vec3             ray_to,
    f32              expand,
    const Entity&    entity,
    f32&             out_c,
    vec3&            out_n
  )
  const
  {
    vec3 pos = entity.get_position();
    f32  rad = entity.get_radius();
    // f32  h   = entity.get_height();

    vec2 n_int = VEC2_ZERO;
    f32  c_int = FLT_MAX;
    bool hit2d = collide::ray_exp_cylinder
    (
      ray_from.xz(), ray_to.xz(), pos.xz(), rad, expand, n_int, c_int
    );

    if (!hit2d)
    {
      return false;
    }
    
    out_c = c_int;
    out_n = vec3{n_int.x, 0.0f, n_int.y};
    return true;
  }
};

//==============================================================================
CollisionHit PhysLevel::cylinder_cast_3d
(
  vec3           ray_start,
  vec3           ray_end,
  f32            expand,
  f32            height,
  EntityTypeMask ent_types   /*= ~EntityTypeMask{0}*/,
  Portals*       out_portals /*= nullptr*/
)
const
{
  auto sector_intersector = [height]
  (
    const MapSectors& map,
    vec3              ray_from,
    vec3              ray_to,
    f32               expand,
    SectorID          sid,
    f32&              out_c,
    vec3&             out_n
  )
  ->bool
  {
    // Note: this might be problematic if we accidentlly get just slightly under
    // the floor. That can even happen due to f32 inaccuraccies.
    return intersect_sector_3d_height
    (
      map, ray_from, ray_to, expand, 0, height, sid, out_c, out_n
    );
  };

  CylCastWallIntersector<StairWalkSettings::Disabled> wall_intersector(height);
  CylCastEntityIntersector entity_intersector;

  return phys_helpers::raycast_generic<vec3>
  (
    *this, ray_start, ray_end, expand, ent_types, out_portals, INVALID_WALL_ID,
    wall_intersector, sector_intersector, entity_intersector
  );
}

//==============================================================================
void PhysLevel::move_character
(
  vec3&                        position,
  vec3&                        velocity_og,
  vec3*                        forward,
  f32                          delta_time,
  f32                          radius,
  f32                          height,
  f32                          max_step_height,
  EntityTypeMask               colliders,
  PhysLevel::CollisionListener listener
)
const
{
  // #define NC_PHYS_DBG

  auto sector_intersector = [height]
  (
    const MapSectors& map,
    vec3              ray_from,
    vec3              ray_to,
    f32               expand,
    SectorID          sid,
    f32&              out_c,
    vec3&             out_n
  )
  ->bool
  {
    // Note: this might be problematic if we accidentlly get just slightly under
    // the floor. That can even happen due to f32 inaccuraccies.
    return intersect_sector_3d_height
    (
      map, ray_from, ray_to, expand, 0, height, sid, out_c, out_n
    );
  };

  // We limit the amount of iterations.
  // Note to self:
  // I thing that this might cause problems around smooth corners, but
  // it is questionable if such a case can happen in the game.
  constexpr u32 MAX_ITERATIONS = 6;

  // Note: we currently do not modify the velocity and that is not good.
  vec3 velocity = velocity_og * delta_time;

  CylCastWallIntersector<StairWalkSettings::Enabled> wall_intersector
  (
    height, max_step_height
  );

  CylCastEntityIntersector entity_intersector;

  CollisionHit last_hit = CollisionHit::no_hit();

  // First check collisions and adjust the velocity
  u32 iterations_left = MAX_ITERATIONS; 
  while(iterations_left-->0)
  {
    using SectorHitType = CollisionHit::SectorHitType;

    const vec3 ray_from = position;
    const vec3 ray_to   = position + velocity;
    const vec3 ray_dir  = ray_to   - ray_from;

    CollisionHit hit = phys_helpers::raycast_generic<vec3>
    (
      *this, ray_from, ray_to, radius, colliders, nullptr,
      INVALID_WALL_ID, wall_intersector, sector_intersector, entity_intersector
    );
    last_hit = hit;

    if (!hit)
    {
      // No hit, go on!
#ifdef NC_PHYS_DBG
      [[maybe_unused]] CollisionHit hit2 = phys_helpers::raycast_generic<vec3>
      (
        *this, ray_from, ray_to, radius, colliders, nullptr,
        INVALID_WALL_ID, wall_intersector, sector_intersector, entity_intersector
      );
#endif

      break;
    }

#ifdef NC_PHYS_DBG
    [[maybe_unused]] CollisionHit hit2 = phys_helpers::raycast_generic<vec3>
    (
      *this, ray_from, ray_to, radius, colliders, nullptr,
      INVALID_WALL_ID, wall_intersector, sector_intersector, entity_intersector
    );
#endif

    nc_assert(is_normal(hit.normal), "Bad things can happen");
    const auto remaining = velocity * (1.0f - hit.coeff);
    const auto projected = hit.normal * dot(remaining, hit.normal);

    velocity -= projected;

    // Let the listener know
    if (listener)
    {
      listener(hit);
    }
  }

  // Then handle nuclidean portal transitions - check which portals
  // we have traversed through and store them.
  Portals portals;
  const auto ray_from = position.xz();
  const auto ray_to   = (position + velocity).xz();
  this->ray_cast_2d(ray_from, ray_to, 0, &portals);

  // Now that we have the portals stored we iterate them and transform our
  // position/direction with the portals
  const bool should_transform = portals.size();
  mat4 transformation = identity<mat4>();
  for (const auto&[wid, sid] : portals)
  {
    const auto pp_trans = map.calc_portal_to_portal_projection(sid, wid);
    transformation = pp_trans * transformation;
  }

  if (should_transform)
  {
    velocity_og = (transformation * vec4{velocity_og, 0.0f}).xyz();
    position    = (transformation * vec4{position,    1.0f}).xyz();
    velocity    = (transformation * vec4{velocity,    0.0f}).xyz();

    if (forward)
    {
      *forward = (transformation * vec4{*forward, 0.0f}).xyz();
    }
  }

  // Note: this is questionable.. Do we add the velocity now, or before we
  // transform it? This might be a source of a potential problem in the future,
  // but for now it seems to be ok.
  // Answer: this is actually ok, as we modify the velocity with the portal
  // transform matrix. Unless?
  position += velocity;

  // Now handle the height and y-velocity.
  // We find the maximal height of the floor and minimal height of the ceiling
  vec2  bbox_offset = vec2{radius, radius};
  vec2  pos2        = position.xz();
  aabb2 bbox        = aabb2{pos2 + bbox_offset, pos2 - bbox_offset};
  std::set<SectorID> nearby_sectors;
  map.sector_grid.query_aabb(bbox, [&](aabb2, SectorID sid)
  {
    nearby_sectors.insert(sid);
    return false;
  });

  // Can't be 1 because then we were sometimes able to climb extra high stairs.
  constexpr f32 DIST_COEFF = 0.99f;

  // Iterate the sectors and check if we touch them
  f32 floor_h = -FLT_MAX;
  f32 ceil_h  =  FLT_MAX;
  for (const SectorID sid : nearby_sectors)
  {
    nc_assert(map.is_valid_sector_id(sid));
    if (map.distance_from_sector_2d(pos2, sid) < radius * DIST_COEFF)
    {
      const SectorData& sd = map.sectors[sid];
      floor_h = std::max(floor_h, sd.floor_height);
      ceil_h  = std::min(ceil_h,  sd.ceil_height);
    }
  }

  // Now adjust the height
  // Hit ceil
  if (ceil_h != FLT_MAX && position.y >= ceil_h - height)
  {
    position.y    = ceil_h - height;
    velocity_og.y = std::min(0.0f, velocity_og.y);
  }

  // Hit floor
  if (floor_h != FLT_MIN && position.y <= floor_h)
  {
    position.y    = floor_h;
    velocity_og.y = std::max(0.0f, velocity_og.y);
  }
}

//==============================================================================
void PhysLevel::move_particle
(
  vec3&                        position,
  vec3&                        velocity_og,
  mat4&                        transform,
  f32&                         delta_time,
  f32                          radius,
  f32                          height,
  f32                          neg_height,
  f32                          bounce,
  EntityTypeMask               colliders,
  PhysLevel::CollisionListener listener /*= nullptr*/
)
const
{
  nc_assert(bounce >= 0.0f, "Invalid range.");

	const vec3 h_offset = -UP_DIR * neg_height;

  f32 total_distance     = length(velocity_og) * delta_time;
	f32 remaining_distance = total_distance;

  while(remaining_distance > 0.0f)
  {
    using SectorHitType = CollisionHit::SectorHitType;

    vec3 velocity = normalize_or_zero(velocity_og) * remaining_distance;
    vec3 ray_from = position + h_offset;
    vec3 ray_to   = position + velocity;
    vec3 ray_dir  = ray_to   - ray_from;

    // Collect the portals along the way we move
    Portals portals_traversed;

    // Cast a ray in a direction of our movement
    CollisionHit hit = this->cylinder_cast_3d
    (
      ray_from, ray_to, radius, height, colliders, &portals_traversed
    );

    // Move. Full distance if we did not hit anything, partial distance if we did
    f32  move_coeff = hit ? hit.coeff  : 1.0f;
    vec3 off_n      = hit ? hit.normal : vec3{0.0f};
    position += ray_dir * move_coeff + off_n * 0.01f;

    // Shorten the distance we want to travel
    remaining_distance -= length(ray_dir) * move_coeff;

    // Now that we have the portals stored we iterate them and transform our
    // position/direction with the portals
    mat4 transform_this_iteration = identity<mat4>();
    for (const auto&[wid, sid] : portals_traversed)
    {
      mat4 portal_transform = map.calc_portal_to_portal_projection(sid, wid);
      transform_this_iteration = portal_transform * transform_this_iteration;
    }

    // Do the actual transformations only if it has an effect
    if (portals_traversed.size())
    {
      velocity_og = transform_this_iteration * vec4{velocity_og, 0.0f};
      position    = transform_this_iteration * vec4{position,    1.0f};
      transform   = transform_this_iteration * transform;
    }

    if (!hit)
    {
      // No hit? Then just end
      break;
    }

    // Let the listener know that we hit something
    if (listener)
    {
      listener(hit);
    }

    nc_assert(is_normal(hit.normal), "Bad things can happen");
    const vec3 reproject = hit.normal * dot(velocity_og, hit.normal);

    // Bounce in the other direction
    velocity_og -= reproject * bounce * 2.0f;
  }
}

}
