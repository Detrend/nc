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
// bool(TVec ray_from, TVec ray_to, f32 expand, const WallData& w1, const SectorData& sector, const WallData& w2, f32& c, f32& n)
template<typename TVec, typename WallHitLambda, typename SectorHitLambda>
RayHit raycast_basic
(
  const PhysLevel&    world,
  TVec                ray_from,
  TVec                ray_to,
  f32                 expand,
  EntityTypeMask      ent_types,
  PhysLevel::Portals* out_portals,
  WallID              ignore_portal,
  WallHitLambda       wall_intersect,
  SectorHitLambda     sector_intersect
)
{
  nc_assert(expand >= 0.0f, "Radius can not be negative!");

  if (ray_from == ray_to)
  {
    // no need to check anything
    return RayHit::no_hit();
  }

  // TODO[perf]: Add a "query_ray" option
  std::set<SectorID> overlap_sectors;
  world.map.sector_grid.query_ray(ray_from, ray_to, expand, [&](aabb2, SectorID sid)
  {
    overlap_sectors.insert(sid);
    return false;
  });

  // Store these for nuclidean portal traversal
  SectorID portal_sector  = INVALID_SECTOR_ID;
  WallID   nc_portal_wall = INVALID_WALL_ID;

  f32  final_coeff  = FLT_MAX;
  vec3 final_normal = vec3{0};

  // Iterate all sectors that the ray might possibly intersect
  for (auto sector_id : overlap_sectors)
  {
    nc_assert(world.map.is_valid_sector_id(sector_id));
    const auto& sector     = world.map.sectors[sector_id];
    const auto  begin_wall = sector.int_data.first_wall;
    const auto  end_wall   = sector.int_data.last_wall;

    f32  c = FLT_MAX;
    vec3 n = vec3{0};
    const bool sector_hit = sector_intersect
    (
      world.map, ray_from, ray_to, expand, sector_id, c, n
    );

    if (sector_hit && c < final_coeff)
    {
      final_coeff  = c;
      final_normal = n;

      // reset the portal info, the floor hit can't be a nc portal
      portal_sector  = INVALID_SECTOR_ID;
      nc_portal_wall = INVALID_WALL_ID;
    }

    // TODO[perf]: In theory, we do not need to check all the walls.
    // Once we hit one all other walls further away can be ignored.
    for (WallID wall_id = begin_wall; wall_id < end_wall; ++wall_id)
    {
      const auto  next_wall_id = map_helpers::next_wall(world.map, sector_id, wall_id);
      const auto& wall_data    = world.map.walls[wall_id];

      const auto portal_type = wall_data.get_portal_type();

      const bool is_normie_portal = portal_type == PortalType::classic;
      const bool is_nc_portal     = portal_type == PortalType::non_euclidean;

      if (is_normie_portal || wall_id == ignore_portal)
      {
        // ignore this portal, because we got here either from a previous
        // iteration or this is a normal portal.
        nc_assert(is_normie_portal || is_nc_portal);
        continue;
      }

      // 0 for nuclidean portals because we do not want to check intersection
      // with NC portals slightly behind us
      const f32 col_exp = is_nc_portal ? 0.0f : expand; 

      c = FLT_MAX;
      n = vec3{0};
      const bool does_intersect = wall_intersect
      (
        world.map, ray_from, ray_to, col_exp, wall_id, next_wall_id, sector_id, c, n
      );

      if (does_intersect && c < final_coeff)
      {
        final_coeff  = c;
        final_normal = n;

        if (is_nc_portal)
        {
          // store the information that this is a portal for later
          portal_sector  = sector_id;
          nc_portal_wall = wall_id;
        }
        else
        {
          // reset the portal info, the closest hit is not a nc portal
          portal_sector  = INVALID_SECTOR_ID;
          nc_portal_wall = INVALID_WALL_ID;
        }
      }
    }
  }

  // Recurse into nuclidean portal if we have to
  if (final_coeff != FLT_MAX && nc_portal_wall != INVALID_WALL_ID)
  {
    nc_assert(final_coeff >= 0.0f && final_coeff <= 1.0f, "NC portal behind us?");

    // Add this portal to the list if we collect them
    if (out_portals)
    {
      out_portals->push_back(PhysLevel::PortalSector
      {
        .wall_id   = nc_portal_wall,
        .sector_id = portal_sector,
      });
    }

    const auto hit_pt = ray_from + (ray_to - ray_from) * final_coeff;
    const auto proj = world.map.calculate_portal_to_portal_projection
    (
      portal_sector,
      nc_portal_wall
    );

    // Calculate new from and to points for the next raycast
    TVec new_from;
    TVec new_to;
    if constexpr (std::is_same_v<TVec, vec3>)
    {
      new_from = (proj * vec4{hit_pt, 1}).xyz();
      new_to   = (proj * vec4{hit_pt, 1}).xyz();
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
      world.map, portal_sector, nc_portal_wall
    );

    // And cast the ray recursively
    const auto hit = raycast_basic<TVec>
    (
      world, new_from, new_to, expand, ent_types,
      out_portals, wall_to_ignore, wall_intersect, sector_intersect
    );

    if (hit)
    {
      // recalculate the out_coeff and out_normal
      final_coeff += (1.0f - final_coeff) * hit.coeff;

      // out normal has to be projected to our space (not the other portal space)
      const auto transform_inv = inverse(proj);
      const auto reproject_norm = (transform_inv * vec4{hit.normal, 0.0f}).xyz();

      final_normal = reproject_norm;
    }
    else
    {
      // we did not hit anything behind the portal
      final_coeff = FLT_MAX;
    }
  }

  // Return true if we hit something
  return RayHit::build(final_coeff, final_normal);
}
}

namespace nc
{ 
  
//==============================================================================
RayHit::operator bool() const
{
  return coeff != FLT_MAX;
}

//==============================================================================
RayHit RayHit::no_hit()
{
  return RayHit{.coeff = FLT_MAX};
}

//==============================================================================
RayHit RayHit::build(f32 c, vec3 n)
{
  return RayHit
  {
    .coeff  = c,
    .normal = n
  };
}

//==============================================================================
static bool intersect_wall_2d(
  const MapSectors& map,
  vec2              ray_from,
  vec2              ray_to,
  f32               expand,
  WallID            w1id,
  WallID            w2id,
  SectorID          /*sid*/,
  f32&              out_c,
  vec3&             out_n)
{
  const auto& w1 = map.walls[w1id];
  const auto& w2 = map.walls[w2id];

  vec2 n = vec2{0};
  const bool hit = collide::ray_exp_wall
  (
    ray_from, ray_to, w1.pos, w2.pos, expand, n, out_c
  );

  out_n = vec3{n.x, 0.0f, n.y};

  return hit;
}

//==============================================================================
static bool intersect_sector_2d(
  [[maybe_unused]]const MapSectors& map,
  [[maybe_unused]]vec2              from,
  [[maybe_unused]]vec2              to,
  [[maybe_unused]]f32               expand,
  [[maybe_unused]]SectorID          sid,
  [[maybe_unused]]f32&              out_c,
  [[maybe_unused]]vec3&             out_n)
{
  return false;
}

//==============================================================================
RayHit PhysLevel::raycast2d_expanded
(
  vec2            from,
  vec2            to,
  f32             expand,
  EntityTypeMask  ent_types,
  PhysLevel::Portals* out_portals
) const
{
  return phys_helpers::raycast_basic<vec2>
  (
    *this, from, to, expand, ent_types,
    out_portals, INVALID_WALL_ID, &intersect_wall_2d, &intersect_sector_2d
  );
}

//==============================================================================
RayHit PhysLevel::raycast2d
(
  vec2            from,
  vec2            to,
  EntityTypeMask  ent_types,
  PhysLevel::Portals* out_portals
) const
{
  return this->raycast2d_expanded(from, to, 0.0f, ent_types, out_portals);
}

//==============================================================================
static bool intersect_wall_3d(
  const MapSectors& map,
  vec3              ray_from,
  vec3              ray_to,
  f32               /*expand*/,
  WallID            w1id,
  WallID            w2id,
  SectorID          sid,
  f32&              out_c,
  vec3&             out_n)
{
  const auto& w1 = map.walls[w1id];
  const auto& w2 = map.walls[w2id];
  const auto& sd = map.sectors[sid];

  const bool hit = intersect::ray_wall_3d
  (
    ray_from, ray_to, w1.pos, w2.pos, sd.floor_height, sd.ceil_height, out_c
  );

  nc_assert(w1.pos != w2.pos);
  auto n2 = flipped(normalize(w2.pos - w1.pos));
  out_n = vec3{n2.x, 0.0f, n2.y};

  return hit;
}

//==============================================================================
static bool intersect_sector_3d(
  const MapSectors& map,
  vec3              from,
  vec3              to,
  f32               /*expand*/,
  SectorID          sid,
  f32&              out_c,
  vec3&             out_n)
{
  const auto& sector = map.sectors[sid];
  const f32 fy = sector.floor_height;
  const f32 cy = sector.ceil_height;

  out_c = FLT_MAX;

  for (auto[floor_y, normal] : {std::pair{fy, UP_DIR}, std::pair{cy, -UP_DIR}})
  {
    if (f32 out; intersect::ray_infinite_plane_xz(from, to, floor_y, out))
    {
      if (map.is_point_in_sector(from + (to - from) * out, sid))
      {
        if (out < out_c)
        {
          out_c = out;
          out_n = normal;
        }
      }
    }
  }

  return out_c != FLT_MAX;
}

//==============================================================================
RayHit PhysLevel::raycast3d
(
  vec3           ray_start,
  vec3           ray_end,
  EntityTypeMask ent_types,
  Portals*       out_portals
) const
{
  return phys_helpers::raycast_basic<vec3>
  (
    *this, ray_start, ray_end, 0.0f, ent_types,
    out_portals, INVALID_WALL_ID, &intersect_wall_3d, &intersect_sector_3d
  );
}

//==============================================================================
void PhysLevel::move_and_collide
(
  vec3&                          position,
  vec3                           velocity,
  vec3&                          forward,
  f32                            radius,
  [[maybe_unused]]f32            height,
  [[maybe_unused]]f32            max_step,
  [[maybe_unused]]EntityTypeMask colliders
) const
{
  constexpr u32 MAX_ITERATIONS = 4;
  constexpr f32 MAX_STEP_SIZE = 0.2f;

  // First check collisions and adjust the velocity
  u32 iterations_left = MAX_ITERATIONS; 
  while(iterations_left-->0)
  {
    if (auto hit = this->raycast2d_expanded(position.xz(), position.xz() + velocity.xz(), radius))
    {
      const auto remaining  = velocity.xz() * (1.0f - hit.coeff);
      const auto projected  = hit.normal.xz() * dot(remaining, hit.normal.xz());
      const auto projected3 = vec3{projected.x, 0.0f, projected.y};
      velocity -= projected3;
    }
    else
    {
      break;
    }
  }

  // Then handle nuclidean portal transitions - check which portals
  // we have traversed through and store them.
  Portals portals;
  const auto ray_from = position.xz();
  const auto ray_to   = (position + velocity).xz();
  this->raycast2d(ray_from, ray_to, 0, &portals);

  // Now that we have the portals stored we iterate them and transform our
  // position/direction with the portals
  const bool should_transform = portals.size();
  mat4 transformation = identity<mat4>();
  for (const auto&[wid, sid] : portals)
  {
    const auto trans = map.calculate_portal_to_portal_projection(sid, wid);
    transformation = trans * transformation;
  }

  vec3 prev_position = position;
  vec3 prev_velocity = velocity;
  vec3 prev_forward = forward;

  if (should_transform)
  {
    position    = (transformation * vec4{position,    1.0f}).xyz();
    velocity    = (transformation * vec4{velocity,    0.0f}).xyz();
    forward     = (transformation * vec4{forward,     0.0f}).xyz();
    position += velocity;
  }
  else
  {
    position += velocity;
  }

  // Change the height as well so we can move up the stairs
  auto sector_id = map.get_sector_from_point(position.xz());
  if (sector_id != INVALID_SECTOR_ID)
  {
    const f32 sector_floor_y = map.sectors[sector_id].floor_height;
    if (position.y < sector_floor_y)
    {
      if (position.y + MAX_STEP_SIZE < sector_floor_y)
      {
        position = prev_position;
        velocity = vec3(0);
        forward = prev_forward;
        return;
      }

      position.y = sector_floor_y;
    }
  }
}

}

