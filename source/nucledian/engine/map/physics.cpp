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
CollisionHit raycast_basic
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
  using SecHitType = CollisionHit::SectorHitType;
  using HitType    = CollisionHit::HitType;

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
  auto add_possible_hit = [&best_hit](const CollisionHit& hit)
  {
    if (hit < best_hit)
    {
      best_hit = hit;
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
        .type            = SecHitType::floor, // TODO: add option for ceiling
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
        SecHitType hit_type = is_nc_hit ? SecHitType::nuclidean_wall : SecHitType::wall;
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
        // add_possible_hit(RayHit::build(c, n, false));
      }
    }
  }

  // Recurse into nuclidean portal if we have to
  if (best_hit && best_hit.type == HitType::sector
    && best_hit.hit.sector.type & SecHitType::nuclidean)
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
    const auto proj = world.map.calculate_portal_to_portal_projection
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
      world.map, best_hit.hit.sector.sector_id, best_hit.hit.sector.wall_id
    );

    // And cast the ray recursively
    const CollisionHit hit = raycast_basic<TVec>
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
  if (ray_from == ray_to)
  {
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

  nc_hit = portal_type == PortalType::non_euclidean;

  // Modify the expansion for non euclidean portals
  f32 exp_modified = nc_hit ? 0.0f : expand;

  vec2 n = vec2{0};
  const bool hit = collide::ray_exp_wall
  (
    ray_from, ray_to, w1.pos, w2.pos, exp_modified, n, out_c
  );

  out_n = vec3{n.x, 0.0f, n.y};

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
CollisionHit PhysLevel::raycast2d_expanded
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
    *this, from, to, expand, ent_types, out_portals, INVALID_WALL_ID,
    &intersect_wall_2d, &intersect_sector_2d, &intersect_entity_empty<vec2>
  );
}

//==============================================================================
CollisionHit PhysLevel::raycast2d
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

  if (from == to)
  {
    return false;
  }

  const SectorData& sector = map.sectors[sid];
  f32 fy = sector.floor_height + y_floor_add;
  f32 cy = sector.ceil_height  - y_ceil_sub;

  out_c = FLT_MAX;

  for (auto[floor_y, normal] : {std::pair{fy, UP_DIR}, std::pair{cy, -UP_DIR}})
  {
    if (f32 out; collide::ray_plane_xz(from, to, floor_y, out))
    {
      f32 min_c = -(y_floor_add + y_ceil_sub) / length(to - from);
      f32 max_c = 1.0f;

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
CollisionHit PhysLevel::raycast3d
(
  vec3           ray_start,
  vec3           ray_end,
  EntityTypeMask ent_types,
  Portals*       out_portals
) const
{
  return phys_helpers::raycast_basic<vec3>
  (
    *this, ray_start, ray_end, 0.0f, ent_types, out_portals, INVALID_WALL_ID,
    &intersect_wall_3d, &intersect_sector_3d, &intersect_entity_empty<vec3>
  );
}

//==============================================================================
void PhysLevel::move_and_collide
(
  vec3&                          position,
  vec3&                          velocity_og,
  vec3*                          forward,
  f32                            delta_time,
  f32                            radius,
  f32                            height,
  f32                            max_step_height,
  [[maybe_unused]]EntityTypeMask colliders,
  [[maybe_unused]]EntityTypeMask report_only,
  [[maybe_unused]]f32            bounce,
  PhysLevel::CollisionListener   listener
) const
{
  // We limit the amount of iterations.
  // Note to self:
  // I thing that this might cause problems around smooth corners, but
  // it is questionable if such a case can happen in the game.
  constexpr u32 MAX_ITERATIONS = 6;

  f32 ground_height = position.y;

  auto wall_intersector = [max_step_height, height, &ground_height]
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
    bool&             nc_hit_out
  )
  ->bool
  {
    if (ray_from.xz() == ray_to.xz())
    {
      return false;
    }

    const WallData&   w1 = map.walls[wid1];
    const WallData&   w2 = map.walls[wid2];
    const SectorData& sc = map.sectors[sid];

    const f32 floor_y = sc.floor_height;
    const f32 ceil_y  = sc.ceil_height;

    // Do the collision checking either way
    vec2 out_n_2d;
    bool hit = collide::ray_exp_wall
    (
      ray_from.xz(), ray_to.xz(), w1.pos, w2.pos,
      expand, out_n_2d, out_c
    );

    // Redirect the normal to 3D
    out_n = vec3{out_n_2d.x, 0, out_n_2d.y};

    // If there is not hit even in 2D then we exit either way
    if (!hit)
    {
      return false;
    }

    // Now split the cases - it is either a full wall, or only a partial one.
    // In both cases we can do 2D raycast and then transform it into the
    // 3D world.
    auto portal_type = w1.get_portal_type();

    // Full - if the collision happens then it is over
    if (portal_type == PortalType::none /*|| out_c <= 0.0f*/)
    {
      // out_n and out_c are already set up properly
      // The second condition evaluates to true if we are already inside
      // a wall slightly.
      return true;
    }

    // Partial - have to check if there is a free window
    nc_assert(map.is_valid_sector_id(w1.portal_sector_id));
    const SectorData& neighbor = map.sectors[w1.portal_sector_id];

    // Calculate the size of the window we can potentially fit into
    f32 window_from_y = std::max(neighbor.floor_height, floor_y);
    f32 window_to_y   = std::min(neighbor.ceil_height,  ceil_y) - height;

    // We did hit the wall.. Check if we can climb the stairs
    f32 hit_y = ray_from.y + (ray_to.y - ray_from.y) * out_c;

    if (hit_y >= window_from_y && hit_y <= window_to_y)
    {
      // We hit the window, continue in the path of the ray..
      if (portal_type != PortalType::non_euclidean) [[likely]]
      {
        // No hit
        return false;
      }

      // Check for nc hit as well
      nc_hit_out = collide::ray_exp_wall
      (
        ray_from.xz(), ray_to.xz(), w1.pos, w2.pos,
        0.0f, out_n_2d, out_c
      );
      out_n = vec3{out_n_2d.x, 0, out_n_2d.y};

      return nc_hit_out;
    }

    // We hit the wall
    return true;
  };

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
    return intersect_sector_3d_height(map, ray_from, ray_to, expand, 0, height, sid, out_c, out_n);
  };

  auto entity_intersector = [&]
  (
    [[maybe_unused]] const PhysLevel& lvl,
    [[maybe_unused]] vec3             ray_from,
    [[maybe_unused]] vec3             ray_to,
    [[maybe_unused]] f32              expand,
    [[maybe_unused]] const Entity&    entity,
    [[maybe_unused]] f32&             out_c,
    [[maybe_unused]] vec3&            out_n
  )
  ->bool
  {
    // Entity intersection not supported (for now)
    return false;
  };

  // Note: we currently do not modify the velocity and that is not good.
  vec3 velocity = velocity_og * delta_time;

  // First check collisions and adjust the velocity
  u32 iterations_left = MAX_ITERATIONS; 
  while(iterations_left-->0)
  {
    auto hit = phys_helpers::raycast_basic
    (
      *this, position, position + velocity, radius, colliders | report_only,
      nullptr, INVALID_WALL_ID, wall_intersector, sector_intersector, entity_intersector
    );

    if (!hit)
    {
      break;
    }

    nc_assert(is_normal(hit.normal), "Bad things can happen");
    const auto remaining = velocity * (1.0f - hit.coeff);
    const auto projected = hit.normal * dot(remaining, hit.normal);

    velocity -= projected;

    if (listener && listener(hit) == CollisionReaction::stop_simulation)
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
    const auto pp_trans = map.calculate_portal_to_portal_projection(sid, wid);
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

  // Note: this is questionable.. Do we add the velocity now, or before 
  // we transform it? This might be a source of a potential problem in
  // the future, but for now it seems to be ok.
  position += velocity;
}

//==============================================================================
void PhysLevel::move_and_collide
(
  Entity&           ent,
  vec3*             forward,
  f32               delta,
  CollisionListener listener
)
{
  nc_assert(ent.get_physics(), "Requires a physics component");

  vec3 pos  = ent.get_position();
  f32  step = ent.get_physics()->max_step_height;
  f32  r    = ent.get_radius();
  f32  h    = ent.get_height();
  f32  b    = ent.get_physics()->bounce;

  auto collide = ent.get_physics()->collide_with;
  auto report  = ent.get_physics()->report_only;

  this->move_and_collide
  (
    pos, ent.get_physics()->velocity, forward, delta,
    r, h, step, collide, report, b, listener
  );
}

}

