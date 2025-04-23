// Project Nucledian Source File
#include <engine/map/map_system.h>
#include <common.h>

#include <aabb.h>

#include <math/utils.h>
#include <math/vector.h>
#include <math/lingebra.h>

#include <grid.h>

#include <algorithm>
#include <array>
#include <vector>
#include <set>
#include <map>
#include <iterator>   // std::back_inserter
#include <cmath>      // std::acos
#include <utility>    // std::pair

#ifdef NC_BENCHMARK
#include <benchmark/benchmark.h>
#include <numbers>
#endif

namespace nc
{

namespace map_helpers
{

//==============================================================================
WallID next_wall(const MapSectors& map, SectorID sector, WallID wall)
{
  NC_ASSERT(sector < map.sectors.size());
  NC_ASSERT(wall < map.sectors[sector].int_data.last_wall);
  WallID next = wall+1;

  if (next == map.sectors[sector].int_data.last_wall)
  {
    next = map.sectors[sector].int_data.first_wall;
  }

  return next;
}

WallID get_wall_id(const MapSectors& map, SectorID sector_id, WallRelID relative_wall_id)
{
  NC_ASSERT(sector_id < map.sectors.size());
  const SectorIntData& sector_data = map.sectors[sector_id].int_data;

  NC_ASSERT(relative_wall_id < (sector_data.last_wall - sector_data.first_wall));
  const WallID wall_id = sector_data.first_wall + relative_wall_id;

  NC_ASSERT(wall_id < sector_data.last_wall);
  return wall_id;
}

//==============================================================================
// calls lambda(PortalID portal_id, WallID wall_index)
template<typename F>
bool for_each_portal(const MapSectors& map, SectorID sector_id, F&& lambda)
{
  NC_ASSERT(sector_id < map.sectors.size());
  const auto& repr = map.sectors[sector_id].int_data;

  bool at_least_one = false;
  for (auto wid = repr.first_wall; wid < repr.last_wall; ++wid)
  {
    NC_ASSERT(wid < map.walls.size());

    if (map.walls[wid].get_portal_type() != PortalType::none)
    {
      at_least_one = true;
      lambda(wid);
    }
  }

  return at_least_one;
}

//==============================================================================
template<typename F>
void for_each_wall(const MapSectors& map, SectorID sector_id, F&& lambda)
{
  NC_ASSERT(sector_id < map.sectors.size());
  const auto& repr = map.sectors[sector_id].int_data;

  for (auto wid = repr.first_wall; wid < repr.last_wall; ++wid)
  {
    NC_ASSERT(wid < map.walls.size());

    const auto nextid = next_wall(map, sector_id, wid); // id of the next wall

    lambda(wid, nextid);
  }
}

//==============================================================================
u32 get_sectors_from_point(const MapSectors& map, vec2 point, SectorID* sectors_out, u32 max_sectors_out)
{
  NC_ASSERT(sectors_out && max_sectors_out);
  u32 counter = 0;

  auto check_overlap = [](const MapSectors& map, SectorID sector_id, vec2 pt) -> bool
  {
    const auto& sector = map.sectors[sector_id];

    const auto first = sector.int_data.first_wall;
    const auto last  = sector.int_data.last_wall;

    NC_ASSERT(first < map.walls.size());
    const auto p1 = map.walls[first].pos;

    for (WallID wall_index = first+1; wall_index < last-1; ++wall_index)
    {
      WallID next_index = wall_index+1;
      const auto p2 = map.walls[wall_index].pos;
      const auto p3 = map.walls[next_index].pos;

      if (intersect::point_triangle(pt, p1, p2, p3))
      {
        return true;
      }
    }

    return false;
  };

  map.sector_grid.query_point(point, [&](aabb2, SectorID id)->bool
  {
    if (check_overlap(map, id, point))
    {
      if (counter < max_sectors_out)
      {
        sectors_out[counter] = id;
      }

      counter += 1;
    }

    return false;
  });

  return counter;
}

//==============================================================================
void modify_nuclidean_frustum(
  const MapSectors& map,
  Frustum2&         frustum,
  WallID            in_portal,
  SectorID          in_sector)
{
  const auto transform = map.calculate_portal_to_portal_projection(in_sector, in_portal);
  const auto new_pos   = (transform * vec4{frustum.center.x, 0.0f, frustum.center.y, 1.0f}).xz();
  const auto new_dir   = (transform * vec4{frustum.direction.x, 0.0f, frustum.direction.y, 0.0f}).xz();
  NC_ASSERT(is_normal(new_dir));

  frustum.center    = new_pos;
  frustum.direction = new_dir;
}

}

//==============================================================================
void MapSectors::query_visible_sectors(
  vec2            position,
  vec2            view_dir,
  f32             hor_fov,
  TraverseVisitor visitor) const
{
  NC_ASSERT(is_normal(view_dir));

  const auto angle = hor_fov >= 180.0f
    ? Frustum2::FULL_ANGLE
    : std::cosf(hor_fov * 0.5f);

  const auto frustum = Frustum2
  {
    .center    = position,
    .direction = view_dir,
    .angle     = angle
  };

  constexpr u64 MAX_CAMERA_SECTORS = 8; // bump this up if the assert ever fires
  std::array<SectorID, MAX_CAMERA_SECTORS> sectors_out;

  const u32 sec_count = map_helpers::get_sectors_from_point(
    *this,
    position,
    sectors_out.data(),
    MAX_CAMERA_SECTORS);

  NC_ASSERT(sec_count <= MAX_CAMERA_SECTORS);

  const auto slots_temp = FrustumBuffer{frustum};

  this->query_visible_sectors_impl(
    sectors_out.data(), sec_count, slots_temp, visitor);
}

//==============================================================================
void MapSectors::query_visible_sectors_impl(
  const SectorID*      start_sector,
  u32                  start_sector_cnt,
  const FrustumBuffer& input_frustums,
  TraverseVisitor      visitor,
  u8                   recursion_depth,
  WallID               source_portal) const
{
  NC_ASSERT(visitor);
  NC_ASSERT(input_frustums.frustum_slots[0] != INVALID_FRUSTUM);

  if (recursion_depth == 0 || start_sector_cnt == 0)
  {
    // exit, the recursion is over or the sector is invalid
    return;
  }

  // [Performance]According to a benchmark, the std::map here is actually
  // faster than using std::unordered_map. Maybe due to poor cache locality?
  // Did not investigate further.
  std::map<SectorID, FrustumBuffer> curr_iteration;
  std::map<SectorID, FrustumBuffer> next_iteration;

  struct NucPortalStruct
  {
    SectorID      sector;
    FrustumBuffer buffer;
  };
  std::map<WallID, NucPortalStruct> nuclidean_portals;

  const auto* begin_sector = start_sector;
  const auto* end_sector   = begin_sector + start_sector_cnt;

  for (u32 i = 0; i < start_sector_cnt; ++i)
  {
    next_iteration.insert({start_sector[i], input_frustums});
  }

  // Now do a BFS in our dimension
  while (next_iteration.size())
  {
    //swap the buffers
    curr_iteration = std::move(next_iteration);
    next_iteration.clear();

    for (const auto&[id, content] : curr_iteration)
    {
      NC_ASSERT(id < sectors.size());

      // Iterate all frustums in this dir
      for (const auto& frustum : content.frustum_slots)
      {
        if (frustum == INVALID_FRUSTUM)
        {
          // this frustum was not filled in
          continue;
        }

        // call the visitor for each existing frustum
        visitor(id, frustum, source_portal);

        // now traverse all portals of the sector and check if we can slide
        // into a neighbor sector
        map_helpers::for_each_portal(*this, id, [&](WallID wall1_idx)
        {
          const auto wall2_idx    = map_helpers::next_wall(*this, id, wall1_idx);
          const auto next_sector  = walls[wall1_idx].portal_sector_id;
          const bool is_nuclidean = walls[wall1_idx].get_portal_type() == PortalType::non_euclidean;
          NC_ASSERT(next_sector != INVALID_SECTOR_ID);

          const auto p1 = walls[wall1_idx].pos;
          const auto p2 = walls[wall2_idx].pos;

          const auto p1_to_p2  = p2-p1;
          const auto p1_to_cam = frustum.center-p1;

          if (std::find(begin_sector, end_sector, next_sector) != end_sector)
          {
            // If the camera is positioned EXACTLY over the border of 2 visible_sectors
            // then we would jump back and forth between these two, because
            // each one would be visible from the second one.
            // These visible_sectors have been visited in the first iteration and we will
            // not return to them.
            // [Performance]: the list can be kept sorted, but usually there
            // will be only 1-4 visible_sectors
            return;
          }

          if (cross(p1_to_cam, p1_to_p2) > 0.0f)
          {
            // early exit, the wall is turned away from the camera
            // The comparison has to be > instead of >=, because that would
            // report the sector we are on a border of as invisible.
            return;
          }

          if (!frustum.intersects_segment(p1, p2))
          {
            // early exit, we do not see the portal
            return;
          }

          auto new_frustum = frustum.modified_with_portal(p1, p2);
          if (new_frustum.is_empty())
          {
            // not sure how the hell this can happen..
            return;
          }

          // rotate the nucledean frustum and shift it relatively to the portal's view
          if (is_nuclidean) [[unlikely]]
          {
            const SectorID out_sector = next_sector;

            // weird non-euclidean portal, store for later
            map_helpers::modify_nuclidean_frustum(*this, new_frustum, wall1_idx, id);

            // either creates and inserts or just merges in
            if (nuclidean_portals.contains(wall1_idx))
            {
              NC_ASSERT(nuclidean_portals[wall1_idx].sector == out_sector);
            }

            nuclidean_portals[wall1_idx].buffer.insert_frustum(new_frustum);
            nuclidean_portals[wall1_idx].sector = out_sector;
          }
          else
          {
            // either creates and inserts or just merges in
            next_iteration[next_sector].insert_frustum(new_frustum);
          }
        });
      }
    }
  }

  // Now lets go to other dimensions in DFS manner
  for (const auto&[portal_id, data] : nuclidean_portals)
  {
    this->query_visible_sectors_impl(&data.sector, 1, data.buffer, [&](SectorID sid, Frustum2 f, WallID pid)
    {
      visitor(sid, f, pid);
    }, recursion_depth-1, portal_id);
  }
}

//==============================================================================
bool MapSectors::for_each_portal_of_sector(
  SectorID      sector,
  WallVisitor visitor) const
{
  return map_helpers::for_each_portal(*this, sector, visitor);
}

//==============================================================================
SectorID MapSectors::get_sector_from_point(vec2 point) const
{
  SectorID sector = INVALID_SECTOR_ID;
  map_helpers::get_sectors_from_point(*this, point, &sector, 1);
  return sector;
}

//==============================================================================
mat4 MapSectors::calculate_portal_to_portal_projection(
  SectorID sector_from,
  WallID   wall_from1) const
{
  NC_ASSERT(sector_from < this->sectors.size());
  NC_ASSERT(wall_from1   < this->walls.size());

  const auto& wall   = this->walls[wall_from1];
  SectorID sector_to = wall.portal_sector_id;

  NC_ASSERT(sector_to != INVALID_SECTOR_ID);
  NC_ASSERT(wall.get_portal_type() == PortalType::non_euclidean);

  const auto wall_from2 = map_helpers::next_wall(*this, sector_from, wall_from1);

  const WallID wall_to1 = this->sectors[sector_to].int_data.first_wall + wall.nc_portal_wall_id;
  const WallID wall_to2 = map_helpers::next_wall(*this, sector_to, wall_to1);

  const auto from_p1 = this->walls[wall_from1].pos;
  const auto from_p2 = this->walls[wall_from2].pos;
  const auto to_p1   = this->walls[wall_to1  ].pos;
  const auto to_p2   = this->walls[wall_to2  ].pos;

  NC_ASSERT(from_p1 != from_p2);
  NC_ASSERT(to_p1   != to_p2);

  auto calc_wall_space_2_world_space = [](vec2 p1, vec2 p2, f32 h) -> mat4
  {
    const vec3 pos3 = vec3{p1.x, h, p1.y};
    const vec2 dir2 = normalize(p2 - p1);
    const vec2 nor2 = -flipped(dir2);

    return mat4
    {
      vec4{dir2.x, 0.0f, dir2.y, 0.0f},
      vec4{0.0f,   1.0f, 0.0f,   0.0f},
      vec4{nor2.x, 0.0f, nor2.y, 0.0f},
      vec4{pos3,                 1.0f},
    };
  };

  const mat4 wall_from_space_2_world_space
    = calc_wall_space_2_world_space(from_p1, from_p2, SECTOR_CEILING_Y);

  const mat4 world_space_2_wall_from_space =
    inverse(wall_from_space_2_world_space);

  const mat4 wall_to_space_2_world_space
    = calc_wall_space_2_world_space(to_p2, to_p1, SECTOR_CEILING_Y);

  const mat4 transition_matrix
    = wall_to_space_2_world_space * world_space_2_wall_from_space;
  return transition_matrix;
}

//==============================================================================
void MapSectors::sector_to_vertices(
  SectorID           sector_id,
  std::vector<vec3>& vertices_out) const
{
  NC_ASSERT(sector_id < this->sectors.size());

  const auto& sector_data = this->sectors[sector_id].int_data;

  // build the floor first from the first point
  const auto first_wall_idx = sector_data.first_wall;
  const auto last_wall_idx  = sector_data.last_wall;
  const auto first_wall_pos = this->walls[first_wall_idx].pos;
  const auto first_pt       = vec3{first_wall_pos.x, 0.0f, first_wall_pos.y};
  NC_ASSERT(first_wall_idx < last_wall_idx);

  for (WallID idx = first_wall_idx + 1; idx < last_wall_idx-1; ++idx)
  {
    const auto next_idx = map_helpers::next_wall(*this, sector_id, idx);
    const auto w1pos    = this->walls[idx     ].pos;
    const auto w2pos    = this->walls[next_idx].pos;
    const auto pt1      = vec3{w1pos.x, SECTOR_FLOOR_Y, w1pos.y};
    const auto pt2      = vec3{w2pos.x, SECTOR_FLOOR_Y, w2pos.y};

    // build floor triangle from the first point and 2 others
    vertices_out.push_back(with_y(first_pt, SECTOR_FLOOR_Y));
    vertices_out.push_back(VEC3_Y);
    vertices_out.push_back(pt2);
    vertices_out.push_back(VEC3_Y);
    vertices_out.push_back(pt1);
    vertices_out.push_back(VEC3_Y);
  }

  // then build ceiling
  for (WallID idx = first_wall_idx + 1; idx < last_wall_idx-1; ++idx)
  {
    const auto next_idx = map_helpers::next_wall(*this, sector_id, idx);
    const auto w1pos    = this->walls[idx     ].pos;
    const auto w2pos    = this->walls[next_idx].pos;
    const auto pt1      = vec3{w1pos.x, SECTOR_CEILING_Y, w1pos.y};
    const auto pt2      = vec3{w2pos.x, SECTOR_CEILING_Y, w2pos.y};

    // build floor triangle from the first point and 2 others
    vertices_out.push_back(with_y(first_pt, SECTOR_CEILING_Y));
    vertices_out.push_back(-VEC3_Y);
    vertices_out.push_back(pt1);
    vertices_out.push_back(-VEC3_Y);
    vertices_out.push_back(pt2);
    vertices_out.push_back(-VEC3_Y);
  }

  // build walls
  #if 1
  for (WallID idx = first_wall_idx; idx < last_wall_idx; ++idx)
  {
    const auto& wall = this->walls[idx];
    if (wall.portal_sector_id != INVALID_SECTOR_ID)
    {
      // is portal
      continue;
    }

    const auto next_idx = map_helpers::next_wall(*this, sector_id, idx);

    const auto w1pos = this->walls[idx].pos;
    const auto w2pos = this->walls[next_idx].pos;

    const auto p1_to_p2 = w1pos != w2pos ? normalize(w2pos - w1pos) : vec2{1, 0};
    const auto flp         = flipped(p1_to_p2);
    const auto wall_normal = vec3{flp.x, 0.0f, flp.y};

    const auto f1 = vec3{w1pos.x, SECTOR_FLOOR_Y, w1pos.y};
    const auto f2 = vec3{w2pos.x, SECTOR_FLOOR_Y, w2pos.y};
    const auto c1 = vec3{w1pos.x, SECTOR_CEILING_Y,  w1pos.y};
    const auto c2 = vec3{w2pos.x, SECTOR_CEILING_Y,  w2pos.y};

    vertices_out.push_back(f1);
    vertices_out.push_back(wall_normal);
    vertices_out.push_back(f2);
    vertices_out.push_back(wall_normal);
    vertices_out.push_back(c1);
    vertices_out.push_back(wall_normal);

    vertices_out.push_back(c1);
    vertices_out.push_back(wall_normal);
    vertices_out.push_back(f2);
    vertices_out.push_back(wall_normal);
    vertices_out.push_back(c2);
    vertices_out.push_back(wall_normal);
  }
  #endif
}

//==============================================================================
bool MapSectors::raycast2d_expanded(
  vec2     from,
  vec2     to,
  f32      expand,
  vec2&    out_normal,
  f32&     out_coeff,
  Portals* out_portals) const
{
  NC_ASSERT(expand >= 0.0f);

  if (from == to)
  {
    return false;
  }

  auto bbox = aabb2{from, to};
  bbox.insert_point(bbox.min - vec2{expand});
  bbox.insert_point(bbox.max + vec2{expand});

  const auto raycast_direction = to - from;

  // TODO[perf]: Add a "query_ray" option
  std::set<SectorID> overlap_sectors;
  this->sector_grid.query_aabb(bbox, [&](aabb2 /*bb*/, SectorID sid)
  {
    overlap_sectors.insert(sid);
    return false;
  });

  SectorID portal_sector  = INVALID_SECTOR_ID;
  WallID   nc_portal_wall = INVALID_WALL_ID;
  out_coeff  = FLT_MAX;
  out_normal = vec2{0};

  for (auto sector_id : overlap_sectors)
  {
    NC_ASSERT(sector_id < this->sectors.size());
    const auto begin_wall = this->sectors[sector_id].int_data.first_wall;
    const auto end_wall   = this->sectors[sector_id].int_data.last_wall;

    // TODO[perf]: In theory, we do not need to check all the walls.
    // Once we hit one all other walls further away can be ignored.
    for (WallID wall_id = begin_wall; wall_id < end_wall; ++wall_id)
    {
      const auto next_wid = map_helpers::next_wall(*this, sector_id, wall_id);
      const auto& w1 = this->walls[wall_id];
      const auto& w2 = this->walls[next_wid];

      const auto portal_type = w1.get_portal_type();

      if (portal_type == PortalType::classic)
      {
        // ignore this wall, it is a portal
        continue;
      }

      const auto p1 = w1.pos;
      const auto p2 = w2.pos;
      NC_ASSERT(p1 != p2);

      const auto wall_normal = flipped(p2 - p1);
      if (dot(wall_normal, raycast_direction) >= 0.0f)
      {
        // ignore wall, it is turned away from us
        continue;
      }

      f32  c = FLT_MAX;
      vec2 n = vec2{0};
      if (intersect::segment_segment_expanded(from, to, p1, p2, expand, n, c) && c < out_coeff)
      {
        out_coeff  = c;
        out_normal = n;

        if (portal_type == PortalType::non_euclidean)
        {
          // store the information that this is a portal
          portal_sector  = sector_id;
          nc_portal_wall = wall_id;
        }
        else
        {
          // reset the portal info
          portal_sector  = INVALID_SECTOR_ID;
          nc_portal_wall = INVALID_WALL_ID;
        }
      }
    }
  }

  if (out_coeff != FLT_MAX && nc_portal_wall != INVALID_WALL_ID)
  {
    NC_ASSERT(out_coeff >= 0.0f && out_coeff <= 1.0f);

    const auto hit_point = from + (to - from) * out_coeff;
    const auto new_from_non_projected = hit_point;
    const auto new_to_non_projected   = to;

    const auto transform = this->calculate_portal_to_portal_projection
    (
      portal_sector,
      nc_portal_wall
    );

    const vec2 new_from
      = (transform * vec4{new_from_non_projected.x, 0.0f, new_from_non_projected.y, 1.0f}).xz();

    const vec2 new_to
      = (transform * vec4{new_to_non_projected.x, 0.0f, new_to_non_projected.y, 1.0f}).xz();

    if (out_portals)
    {
      out_portals->push_back(PortalSector
      {
        .wall_id   = nc_portal_wall,
        .sector_id = portal_sector,
      });
    }

    f32  new_out_coeff;
    vec2 new_out_norm;
    if (this->raycast2d_expanded(new_from, new_to, expand, new_out_norm, new_out_coeff, out_portals))
    {
      // recalculate the out_coeff and out_normal
      out_coeff += (1.0f - out_coeff) * new_out_coeff;

      // out normal has to be projected to our space (not the other portal space)
      const auto transform_inv = inverse(transform);
      const auto reproject_norm
        = (transform_inv * vec4{new_out_norm.x, 0.0f, new_out_norm.y, 0.0f}).xz();

      out_normal = reproject_norm;
    }
    else
    {
      // we did not hit anything behind the portal
      out_coeff = FLT_MAX;
    }
  }

  return out_coeff != FLT_MAX;
}

//==============================================================================
bool MapSectors::raycast3d(vec3 from, vec3 to, vec3& out_normal, f32& out_coeff) const
{
  if (from == to)
  {
    return false;
  }

  const auto bbox = aabb2{from.xz, to.xz};

  // TODO[perf]: Add a "query_ray" option
  std::set<SectorID> overlap_sectors;
  this->sector_grid.query_aabb(bbox, [&](aabb2 /*bb*/, SectorID sid)
  {
    overlap_sectors.insert(sid);
    return false;
  });

  out_coeff  = FLT_MAX;
  out_normal = vec3{0};
  f32 count  = 1.0f;

  auto add_hit = [&out_coeff, &out_normal, &count](f32 coeff, vec3 normal)
  {
    if (coeff == out_coeff)
    {
      out_normal = out_normal + normal;
      count += 1.0f;
    }
    else if (coeff < out_coeff)
    {
      out_coeff  = coeff;
      out_normal = normal;
      count      = 1.0f;
    }
  };

  // check floor
  if (f32 out; intersect::ray_infinite_plane_xz(from, to, SECTOR_FLOOR_Y, out))
  {
    add_hit(out, UP_DIR);
  }

  // check ceiling
  if (f32 out; intersect::ray_infinite_plane_xz(from, to, SECTOR_CEILING_Y, out))
  {
    add_hit(out, -UP_DIR);
  }

  for (auto sector_id : overlap_sectors)
  {
    NC_ASSERT(sector_id < this->sectors.size());
    const auto begin_wall = this->sectors[sector_id].int_data.first_wall;
    const auto end_wall   = this->sectors[sector_id].int_data.last_wall;

    // TODO[perf]: In theory, we do not need to check all the walls.
    // Once we hit one all other walls further away can be ignored.
    for (auto wid = begin_wall; wid < end_wall; ++wid)
    {
      const auto  next_wid = map_helpers::next_wall(*this, sector_id, wid);
      const auto& w1 = this->walls[wid];
      const auto& w2 = this->walls[next_wid];

      const bool is_portal = w1.get_portal_type() == PortalType::classic;
      if (is_portal)
      {
        // Ignore this wall, it is a portal
        continue;
      }

      const auto& p1 = w1.pos;
      const auto& p2 = w2.pos;
      NC_ASSERT(p1 != p2);

      f32  coeff  = FLT_MAX;
      vec2 normal = flipped(normalize(p2 - p1));
      if (intersect::ray_wall(from, to, p1, p2, SECTOR_FLOOR_Y, SECTOR_CEILING_Y, coeff))
      {
        add_hit(coeff, vec3{normal.x, 0.0f, normal.y});
      }
    }
  }

  out_normal = out_normal / count;

  return out_coeff != FLT_MAX;
}

//==============================================================================
PortType WallData::get_portal_type() const
{
  if (this->portal_sector_id == INVALID_SECTOR_ID)
  {
    // this is a wall
    return PortalType::none;
  }
  else if (this->nc_portal_wall_id == INVALID_WALL_REL_ID)
  {
    // normal portal
    return PortalType::classic;
  }
  else
  {
    // nuclidean portal
    return PortalType::non_euclidean;
  }
}

}

namespace nc::map_building
{

//==============================================================================
static void build_sector_grid(MapSectors& map)
{
  vec2 grid_min = vec2{ FLT_MAX};
  vec2 grid_max = vec2{-FLT_MAX};

  for (const auto& wall : map.walls)
  {
    grid_min = min(grid_min, wall.pos);
    grid_max = max(grid_max, wall.pos);
  }

  vec2 size  = grid_max - grid_min;

  u64 sector_count      = map.sectors.size();
  f32 height_to_width   = size.y / size.x;
  u64 width_grid_cells  = sector_count;
  u64 height_grid_cells = static_cast<u64>(sector_count * height_to_width);

  map.sector_grid.initialize(width_grid_cells, height_grid_cells, grid_min, grid_max);

  // and now insert all the visible_sectors
  for (SectorID sid = 0; sid < map.sectors.size(); ++sid)
  {
    const auto& sector = map.sectors[sid];
    aabb2 sector_aabb;

    for (u64 id = sector.int_data.first_wall; id < sector.int_data.last_wall; ++id)
    {
      sector_aabb.insert_point(map.walls[id].pos);
    }

    map.sector_grid.insert(sector_aabb, sid);
  }
}

//==============================================================================
// TODO: we are doing 2x the amount of intersections (each 2 visible_sectors are
// intersected 2 times instead of only once).
// TODO: use some data structure instead of the grid
static bool check_for_sector_overlaps(
  const std::vector<vec2>&            points,
  const std::vector<SectorBuildData>& sectors,
  OverlapInfo&                        overlap)
{
  using std::numeric_limits;

  // the range of the map
  vec2 map_min = vec2{ FLT_MAX,  FLT_MAX};
  vec2 map_max = vec2{-FLT_MAX, -FLT_MAX};

  for (auto&& point : points)
  {
    map_min = min(map_min, point);
    map_max = max(map_max, point);
  }

  constexpr u64 GRID_SIZE = 128;

  StatGridAABB2<SectorID> grid;
  grid.initialize(GRID_SIZE, GRID_SIZE, map_min, map_max);

  // calculate bboxes for the visible_sectors
  std::vector<aabb2> sector_bboxes(sectors.size());
  for (SectorID sector_idx = 0; sector_idx < sectors.size(); ++sector_idx)
  {
    // make an aabb out of the sector points
    aabb2 bbox;

    for (auto&& point : sectors[sector_idx].points)
    {
      bbox.insert_point(points[point.point_index]);
    }

    sector_bboxes[sector_idx] = bbox;
  }

  // populate the grid
  for (SectorID sector_idx = 0; sector_idx < sectors.size(); ++sector_idx)
  {
    const auto& bbox = sector_bboxes[sector_idx];
    NC_ASSERT(bbox.is_valid());

    grid.insert(bbox, sector_idx);
  }

  // check visible_sectors overlap
  for (SectorID sector_idx = 0; sector_idx < sectors.size(); ++sector_idx)
  {
    const auto& bbox = sector_bboxes[sector_idx];
    NC_ASSERT(bbox.is_valid());

    std::set<SectorID> possible_intersection_sectors;
    grid.query_aabb(bbox, [&](aabb2, const SectorID& sector)->bool
    {
      if (sector != sector_idx)
      {
        possible_intersection_sectors.insert(sector);
      }
      return false;
    });

    for (SectorID other_idx : possible_intersection_sectors)
    {
      NC_ASSERT(other_idx != sector_idx);

      const auto& sector1 = sectors[sector_idx];
      const auto& sector2 = sectors[other_idx];

      NC_ASSERT(sector1.points.size() >= 3);
      NC_ASSERT(sector2.points.size() >= 3);

      std::vector<vec2> points1, points2;
      points1.reserve(sector1.points.size());
      points2.reserve(sector2.points.size());

      auto wb_data_to_point = [&](const WallBuildData& wb)->vec2
      {
        return points[wb.point_index];
      };

      std::transform(
        sector1.points.begin(),
        sector1.points.end(),
        std::back_inserter(points1),
        wb_data_to_point);

      std::transform(
        sector2.points.begin(),
        sector2.points.end(),
        std::back_inserter(points2),
        wb_data_to_point);

      if (intersect::convex_convex(points1, points2))
      {
        overlap = OverlapInfo
        {
          .sector1 = sector_idx,
          .sector2 = other_idx,
        };
        return false;
      }
    }
  }

  return true;
}

//==============================================================================
static void resolve_clockwise_wall_order(SectorBuildData& sector)
{
  // just revert the direction of the walls..
  std::reverse(sector.points.begin(), sector.points.end());
}

//==============================================================================
static bool resolve_non_convex_and_clockwise(
  const std::vector<vec2>&      points,
  std::vector<SectorBuildData>& sectors,
  NonConvexInfo&                error)    // we might add or modify this
{
  for (SectorID sector_id = 0; sector_id < sectors.size(); ++sector_id)
  {
    // sum of inner angles
    f32 degree_sum = 0.0f;

    auto& sector = sectors[sector_id];
    // If the shape is convex then it will have only counter-clockwise
    // angles or only clockwise angles
    u32 counter_clockwise_count = 0;
    u32 clockwise_count         = 0;

    // First, check the counter-clockwise ordering.
    // We want the points to be in counter clockwise order. This can be
    // determined by examining the sign of 2D cross product of two wall
    // directions.
    // The sign of the cross product should be positive or zero for
    // clockwise rotated walls.
    for (u32 curr_index = 0; curr_index < sector.points.size(); ++curr_index)
    {
      u32 next_index  = (curr_index+1) % sector.points.size();
      u32 third_index = (next_index+1) % sector.points.size();

      const auto& p1 = points[sector.points[curr_index].point_index];
      const auto& p2 = points[sector.points[next_index].point_index];
      const auto& p3 = points[sector.points[third_index].point_index];

      const auto p1_to_p2 = p2-p1;    // first wall direction
      const auto p2_to_p3 = p3-p2;    // second wall direction

      const auto dot_prod = dot(normalize(p1_to_p2), normalize(p2_to_p3));
      const auto degrees  = rad2deg(std::acos(dot_prod));
      degree_sum += degrees;

      // By using the 2D cross product we determine if two walls
      // are counter clockwise or clockwise
      const f32 sg = sgn(cross(p1_to_p2, p2_to_p3));
      if (sg < 0.0f)
      {
        // this angle between two walls is
        clockwise_count += 1;
      }
      else if (sg > 0.0f)
      {
        counter_clockwise_count += 1;
      }

      if (clockwise_count && counter_clockwise_count)
      {
        // the shape is non convex, we can stop here..
        break;
      }
    }

    if (clockwise_count && counter_clockwise_count)
    {
      // the sector is non-convex, not good
      error = NonConvexInfo
      {
        .sector = sector_id,
      };
      return false;
    }

    // 1/4 degree to account for float inaccuracies
    constexpr f32 MAX_DEGREE_DIFF = 0.25f;
    const auto degree_diff = std::abs(degree_sum - 360.0f);
    if (degree_diff > MAX_DEGREE_DIFF)
    {
      // the sector is apparently degenerated
      error = NonConvexInfo
      {
        .sector = sector_id,
      };
      return false;
    }

    if (clockwise_count)
    {
      // The sector is ok, just the walls have incorrect ordering..
      // Rotate the walls around so that they are in a
      // counter clockwise order
      resolve_clockwise_wall_order(sector);
    }
  }

  return true;
}

//==============================================================================
static std::tuple<vec3, f32, vec3> compute_pos_rotation_scale(
  const MapSectors& map,
  SectorID sector_id,
  WallID wall_id
)
{
  // TODO: replace these later once we can do visible_sectors with varying height
  constexpr f32 FLOOR_Y = 0.0f;
  constexpr f32 CEIL_Y  = 1.5f;

  const WallID next_wall = map_helpers::next_wall(map, sector_id, wall_id);
  const vec2 position1 = map.walls[wall_id].pos;
  const vec2 position2 = map.walls[next_wall].pos;

  const vec2 center_2d = (position1 + position2) * 0.5f;
  const vec3 center = vec3(center_2d.x, FLOOR_Y + (CEIL_Y - FLOOR_Y) / 2.0f, center_2d.y);

  const vec2 direction = normalize(position2 - position1);
  const f32  angle = atan2(direction.y, direction.x);

  const f32 width = length(position2 - position1);
  const f32 height = CEIL_Y - FLOOR_Y;
  const vec3 scale = vec3(width, height, 1.0f);

  return { center, angle, scale };
}

//==============================================================================
static void compute_portal_render_data(MapSectors& map)
{
  for (SectorID sector_id = 0; sector_id < map.sectors.size(); ++sector_id)
  {
    map.for_each_portal_of_sector(sector_id, [&](WallID wall_id)
    {
      if (map.walls[wall_id].get_portal_type() != PortalType::non_euclidean)
        return;

      const auto [src_pos, src_rotation, src_scale] = compute_pos_rotation_scale(map, sector_id, wall_id);
      const mat4 src_transform_no_scale = translate(mat4(1.0f), src_pos) * rotate(mat4(1.0f), src_rotation, VEC3_Y);
      const mat4 src_transform = src_transform_no_scale * scale(mat4(1.0f), src_scale);

      const SectorID dest_sector_id = map.walls[wall_id].portal_sector_id;
      const WallRelID dest_rel_wall_id = map.walls[wall_id].nc_portal_wall_id;
      const WallID dest_wall_id = map_helpers::get_wall_id(map, dest_sector_id, dest_rel_wall_id);
      const auto [dest_pos, dest_rotation, _] = compute_pos_rotation_scale(map, dest_sector_id, dest_wall_id);
      const mat4 dest_transform_no_scale = translate(mat4(1.0f), dest_pos) * rotate(mat4(1.0f), dest_rotation, VEC3_Y);

      map.walls[wall_id].render_data_index = static_cast<PortalRenderID>(map.portals_render_data.size());
      map.portals_render_data.emplace_back(
        src_rotation,
        src_pos,
        src_transform,
        src_transform_no_scale * rotate(mat4(1.0f), PI, VEC3_Y) * inverse(dest_transform_no_scale)
      );
    });
  }
}

//==============================================================================
int build_map(
  const std::vector<vec2>&            points,
  const std::vector<SectorBuildData>& sectors,
  MapSectors&                         output,
  MapBuildFlags                       flags)
{
  using namespace MapBuildFlag;

  // TODO: check if there are not too many walls, portals or walls within a sector
  NC_ASSERT(sectors.size() <= MAX_SECTORS);

  output.sectors.clear();
  output.walls.clear();

  // list of visible_sectors for each point
  std::vector<std::vector<SectorID>> points_to_sectors;
  points_to_sectors.resize(points.size());

  // copy of the original visible_sectors
  auto temp_sectors = sectors;

  [[maybe_unused]]const bool assert_on_check_fail = flags & assert_on_fail;

  const bool do_convexity_check = !(flags & omit_convexity_clockwise_check);
  const bool do_overlap_check   = !(flags & omit_sector_overlap_check);

  // Check for point and sector count
  u64 MAX_U16 = static_cast<u16>(-1);
  NC_ASSERT(points.size()  < MAX_U16);
  NC_ASSERT(sectors.size() < MAX_U16);

  // Check for non convex and non-clockwise order visible_sectors
  if (do_convexity_check)
  {
    if (NonConvexInfo oi; !resolve_non_convex_and_clockwise(points, temp_sectors, oi))
    {
      NC_ASSERT(!assert_on_check_fail);
      return false;
    }
  }

  // Check for sector overlaps
  if (do_overlap_check)
  {
    if (OverlapInfo oi; !check_for_sector_overlaps(points, temp_sectors, oi))
    {
      NC_ASSERT(!assert_on_check_fail);
      return false;
    }
  }

  // store which points are used by which visible_sectors
  for (u64 i = 0; i < temp_sectors.size(); ++i)
  {
    for (auto&& wall : temp_sectors[i].points)
    {
      points_to_sectors[wall.point_index].push_back(static_cast<u16>(i));
    }
  }

  // find portal walls and store them
  for (SectorID sector_id = 0; sector_id < temp_sectors.size(); ++sector_id)
  {
    auto&& sector = temp_sectors[sector_id];
    auto& output_sector = output.sectors.emplace_back();

    const u32 total_wall_count = static_cast<u32>(sector.points.size());

    output_sector.int_data.first_wall = static_cast<WallID>(output.walls.size());
    output_sector.int_data.last_wall  = static_cast<WallID>(
      output_sector.int_data.first_wall + total_wall_count);

    // cycle through all walls and push them into the array
    NC_ASSERT(sector.points.size() <= MAX_WALLS_PER_SECTOR);
    for (WallRelID wall_index = 0; wall_index < sector.points.size(); ++wall_index)
    {
      u16 next_wall_index = (wall_index + 1) % sector.points.size();
      u16 point_index = sector.points[wall_index].point_index;
      u16 next_point_index = sector.points[next_wall_index].point_index;

      auto& neighbor_sectors_of_p1 = points_to_sectors[point_index];
      SectorID portal_with = INVALID_SECTOR_ID;

      // Find if this is a portal wall. We do this by checking if there is any other sector
      // that has a wall on the same made of the same vertices
      for (const u16 used_sector_id : neighbor_sectors_of_p1)
      {
        if (used_sector_id == sector_id)
        {
          // this is us
          continue;
        }

        auto& neighbor_sectors_of_p2 = points_to_sectors[next_point_index];
        const auto& b = neighbor_sectors_of_p2.begin();
        const auto& e = neighbor_sectors_of_p2.end();
        const bool whole_wall = std::find(b, e, used_sector_id) != e;
        if (whole_wall)
        {
          portal_with = used_sector_id;
          break;
        }
      }

      // Check if this is not a nuclidean portal..
      const auto nuclidean_sector_id = sector.points[wall_index].nc_portal_sector_index;
      const bool is_nuclidean_portal = nuclidean_sector_id != INVALID_SECTOR_ID;
      WallRelID nuclidean_wall_rel_idx = INVALID_WALL_REL_ID;
      if (is_nuclidean_portal)
      {
        if (portal_with != INVALID_SECTOR_ID)
        {
          // This happens if the portal is both nuclidean and non-nuclidean..
          // We do not allow this
          NC_ASSERT(!assert_on_check_fail);
          return false;
        }

        portal_with            = nuclidean_sector_id;
        nuclidean_wall_rel_idx = sector.points[wall_index].nc_portal_point_index;
      }

      // Add a new wall
      output.walls.push_back(WallData
      {
        .pos               = points[point_index],
        .portal_sector_id  = portal_with,
        .nc_portal_wall_id = nuclidean_wall_rel_idx,
      });
    }
  }

  // now, there should be the same number of visible_sectors in the output as on the input
  NC_ASSERT(output.sectors.size() == sectors.size());

  // and finally, build the grid
  build_sector_grid(output);

  compute_portal_render_data(output);

  return true;
}

#ifdef NC_BENCHMARK

static void test_make_sector(
  const std::vector<u16>&                     points,
  std::vector<map_building::SectorBuildData>& out)
{
  auto wall_port = WallExtData
  {
    .texture_id = 0,
    .texture_offset_x = 0,
    .texture_offset_y = 0
  };

  auto sector_port = SectorExtData
  {
    .floor_texture_id = 1,
    .ceil_texture_id  = 0,
    .floor_height     = 0,
    .ceil_height      = 13,
  };

  std::vector<map_building::WallBuildData> walls;
  for (auto& p : points)
  {
    walls.push_back(map_building::WallBuildData
    {
      .point_index = p,
      .ext_data = wall_port,
    });
  }
  out.push_back(map_building::SectorBuildData
  {
    .points = std::move(walls),
    .ext_data = sector_port,
  });
}

static void make_random_square_maze_map(MapSectors& map, u32 size, u32 seed)
{
  std::srand(seed);

  std::vector<vec2> points;

  // first up the points
  for (u32 i = 0; i < size; ++i)
  {
    for (u32 j = 0; j < size; ++j)
    {
      const f32 x = (j / static_cast<f32>(size-1)) * 2.0f - 1.0f;
      const f32 y = (i / static_cast<f32>(size-1)) * 2.0f - 1.0f;
      points.push_back(vec2{x, y});
    }
  }

  // and then the visible_sectors
  std::vector<map_building::SectorBuildData> sectors;

  for (u16 i = 0; i < size-1; ++i)
  {
    for (u16 j = 0; j < size-1; ++j)
    {
      auto rng = std::rand();
      if (rng % 4 != 0)
      {
        u16 i1 = static_cast<u16>(i * size + j);
        u16 i2 = i1+1;
        u16 i3 = static_cast<u16>((i+1) * size + j + 1);
        u16 i4 = i3-1;
        test_make_sector({i1, i2, i3, i4}, sectors);
      }
    }
  }

  using namespace map_building::MapBuildFlag;

  map_building::build_map(
    points, sectors, map,
    //omit_convexity_clockwise_check |
    //omit_sector_overlap_check      |
    //omit_wall_overlap_check        |
    assert_on_fail);
}

void benchmark_map_creation_squared(
  benchmark::State&           state,
  map_building::MapBuildFlags flags)
{
  std::vector<vec2> points;

  const u32 SIZE = static_cast<u32>(state.range(0));

  // first up the points
  for (u32 i = 0; i < SIZE; ++i)
  {
    for (u32 j = 0; j < SIZE; ++j)
    {
      const f32 x = (j / static_cast<f32>(SIZE-1)) * 2.0f - 1.0f;
      const f32 y = (i / static_cast<f32>(SIZE-1)) * 2.0f - 1.0f;
      points.push_back(vec2{x, y});
    }
  }

  // and then the visible_sectors
  std::vector<map_building::SectorBuildData> sectors;

  for (u16 i = 0; i < SIZE-1; ++i)
  {
    for (u16 j = 0; j < SIZE-1; ++j)
    {
      u16 i1 = static_cast<u16>(i * SIZE + j);
      u16 i2 = i1+1;
      u16 i3 = static_cast<u16>((i+1) * SIZE + j + 1);
      u16 i4 = i3-1;
      test_make_sector({i1, i2, i3, i4}, sectors);
    }
  }

  using namespace map_building::MapBuildFlag;

  for (auto _ : state)
  {
    MapSectors map;
    map_building::build_map(points, sectors, map, flags);

    benchmark::DoNotOptimize(map);
    benchmark::ClobberMemory();
  }
}

void benchmark_visibility_query(benchmark::State& state)
{
  MapSectors map;
  make_random_square_maze_map(map, static_cast<u32>(state.range(0)), 0);

  std::vector<vec2> points_inside;
  bool has_pt = false;
  vec2 pt = vec2{0};

  while (!has_pt)
  {
    const f32 x = ((std::rand() % 4097) - 2048) / 2048.0f;
    const f32 y = ((std::rand() % 4097) - 2048) / 2048.0f;

    if (map.get_sector_from_point(vec2{x, y}) != INVALID_SECTOR_ID)
    {
      has_pt = true;
      pt = vec2{x, y};
    }
  }

  SectorID last_sector = INVALID_SECTOR_ID;
  for (auto _ : state)
  {
    map.query_visible_sectors(pt, vec2{1, 0}, Frustum2::FULL_ANGLE, [&](SectorID id, Frustum2, WallID)
    {
      last_sector = id;
    });
  }

  benchmark::DoNotOptimize(last_sector);
  benchmark::ClobberMemory();

  //state.SetItemsProcessed(points_inside.size() * state.iterations());
}

constexpr auto OMIT_CHECKS_FLAGS
  = map_building::MapBuildFlag::omit_convexity_clockwise_check
  | map_building::MapBuildFlag::omit_sector_overlap_check;

BENCHMARK_CAPTURE(benchmark_visibility_query,     "Visibility query")                             ->Arg(20)->Unit(benchmark::kNanosecond);
BENCHMARK_CAPTURE(benchmark_visibility_query,     "Visibility query")                             ->Arg(30)->Unit(benchmark::kNanosecond);
BENCHMARK_CAPTURE(benchmark_visibility_query,     "Visibility query")                             ->Arg(40)->Unit(benchmark::kNanosecond);

BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (with checks)", 0)                ->Arg(16)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (no checks)",   OMIT_CHECKS_FLAGS)->Arg(16)->Unit(benchmark::kMillisecond);

BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (with checks)", 0)                ->Arg(32)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (no checks)",   OMIT_CHECKS_FLAGS)->Arg(32)->Unit(benchmark::kMillisecond);

BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (with checks)", 0)                ->Arg(48)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (no checks)",   OMIT_CHECKS_FLAGS)->Arg(48)->Unit(benchmark::kMillisecond);
#endif

}

