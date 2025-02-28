// Project Nucledian Source File
#include <engine/map/map_system.h>

#include <aabb.h>
#include <vec.h>
#include <vector_maths.h>
#include <maths.h>
#include <grid.h>

#include <algorithm>
#include <array>
#include <vector>
#include <set>
#include <map>
#include <iterator>   // std::back_inserter
#include <cmath>      // std::acos

#ifdef NC_BENCHMARK
#include <benchmark/benchmark.h>
#endif

namespace nc
{

namespace map_helpers
{

//==============================================================================
// calls lambda(PortalID portal_id, WallID wall_index)
template<typename F>
bool for_each_portal(const MapSectors& map, SectorID sector_id, F&& lambda)
{
  NC_ASSERT(sector_id < map.sectors.size());
  const auto& repr = map.sectors[sector_id].int_data;

  for (auto pid = repr.first_portal; pid < repr.last_portal; ++pid)
  {
    NC_ASSERT(pid < map.walls.size());

    const auto wall_index = map.portals[pid].wall_index;
    NC_ASSERT(wall_index < repr.last_wall);

    lambda(pid, wall_index);
  }

  return repr.first_portal < repr.last_portal;
}

//==============================================================================
// TODO: an acceleration data structure is a MUST HAVE here!!!
SectorID get_sector_from_point(const MapSectors& map, vec2 point)
{
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

  #ifdef DO_VALIDITY_CHECK
  SectorID check_result = INVALID_SECTOR_ID;
  for (SectorID sector_id = 0; sector_id < map.sectors.size(); ++sector_id)
  {
    if (check_overlap(map, sector_id, point))
    {
      check_result = sector_id;
      break;
    }
  }
  #endif

  SectorID result = INVALID_SECTOR_ID;
  map.sector_grid.query_point(point, [&](aabb2, SectorID id)->bool
  {
    if (check_overlap(map, id, point))
    {
      result = id;
      return true;  // we can stop
    }

    return false;
  });

  #ifdef DO_VALIDITY_CHECK
  NC_ASSERT(result == check_result);
  #endif

  return result;
}

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

}

//==============================================================================
constexpr u64 FRUSTUM_SLOT_CNT = 4;
struct FrustumBuffer
{
  using FrustumArray = std::array<Frustum2, FRUSTUM_SLOT_CNT>;

  FrustumArray frustum_slots;
  PortalID     see_through_portal_id; // Unused for now, will be handy later

  explicit FrustumBuffer(Frustum2 from_frustum)
  : see_through_portal_id(INVALID_PORTAL_ID)
  {
    frustum_slots.fill(INVALID_FRUSTUM);
    frustum_slots[0] = from_frustum;
  }

  // This merges a new frustum with overlapping one. If no overlapping frustum
  // is found then inserts it or merges with a closest one.
  void insert_frustum(Frustum2 new_frustum)
  {
    u64  closest_idx = 0;
    f32  closest_dst = FLT_MAX;
    bool merged      = false;

    for (u64 i = 0; i < FRUSTUM_SLOT_CNT; ++i)
    {
      auto& other_frustum = this->frustum_slots[i];
      auto  is_invalid    = other_frustum == INVALID_FRUSTUM;

      f32 angle_diff = is_invalid ? 0.0f : other_frustum.angle_difference(new_frustum);

      if (angle_diff <= 0.0f)
      {
        // we found an overlapping frustum, lets merge with it
        if (is_invalid)
        {
          other_frustum = new_frustum;
        }
        else
        {
          other_frustum = other_frustum.merged_with(new_frustum);
        }

        merged = true;
        break;
      }

      // not overlapping, but might be quite close
      if (angle_diff < closest_dst)
      {
        angle_diff  = closest_dst;
        closest_idx = i;
      }
    }

    if (!merged)
    {
      NC_ASSERT(closest_idx < FRUSTUM_SLOT_CNT);

      // No overlapping frustum found and all slots are full?
      // Then merge with a closest one
      auto& closest_frustum = this->frustum_slots[closest_idx];
      closest_frustum = closest_frustum.merged_with(new_frustum);
    }
  }
};

//==============================================================================
void MapSectors::query_visible_sectors(
  Frustum2        input_frustum,
  TraverseVisitor visitor) const
{
  NC_ASSERT(visitor);

  SectorID start_sector = this->get_sector_from_point(input_frustum.center);

  if (start_sector == INVALID_SECTOR_ID)
  {
    // This point is outside of all sectors..
    return;
  }

  std::map<SectorID, FrustumBuffer> curr_iteration;
  std::map<SectorID, FrustumBuffer> next_iteration;

  next_iteration.insert({start_sector, FrustumBuffer{input_frustum}});

  // Now do a BFS
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
        visitor(id, frustum);

        // now traverse all portals of the sector and check if we can slide
        // into a neighbor sector
        this->for_each_portal_of_sector(id, [&](PortalID, WallID wall1_idx)
        {
          const auto wall2_idx   = map_helpers::next_wall(*this, id, wall1_idx);
          const auto next_sector = walls[wall1_idx].portal_sector_id;
          NC_ASSERT(next_sector != INVALID_SECTOR_ID);

          const auto p1 = walls[wall1_idx].pos;
          const auto p2 = walls[wall2_idx].pos;

          const auto p1_to_p2  = p2-p1;
          const auto p1_to_cam = frustum.center-p1;

          if (cross(p1_to_cam, p1_to_p2) >= 0.0f)
          {
            // early exit, the wall is turned away from the camera
            return;
          }

          if (!frustum.intersects_segment(p1, p2))
          {
            // early exit, we do not see the portal
            return;
          }

          const auto new_frustum = frustum.modied_with_portal(p1, p2);
          if (new_frustum.is_empty())
          {
            // not sure how the hell this can happen..
            return;
          }

          // visit the sector in the next iteration
          if (auto it = next_iteration.find(next_sector); it != next_iteration.end())
          {
            // did we already reach this sector? Then merge the frustum with some old one
            it->second.insert_frustum(new_frustum);
          }
          else
          {
            next_iteration.insert({next_sector, FrustumBuffer{new_frustum}});
          }
        });
      }

    }
  }
}

//==============================================================================
bool MapSectors::for_each_portal_of_sector(
  SectorID      sector,
  PortalVisitor visitor) const
{
  return map_helpers::for_each_portal(*this, sector, visitor);
}

//==============================================================================
SectorID MapSectors::get_sector_from_point(vec2 point) const
{
  return map_helpers::get_sector_from_point(*this, point);
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

  // and now insert all the sectors
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
// TODO: we are doing 2x the amount of intersections (each 2 sectors are
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

  // calculate bboxes for the sectors
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

  // check sectors overlap
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

  // and make sure that we remap the portable data properly
  for (u32 index = 0; index < sector.points.size(); ++index)
  {
    u32 next_index = (index+1) % sector.points.size();
    std::swap(
      sector.points[index].ext_data,
      sector.points[next_index].ext_data);
  }
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
int build_map(
  const std::vector<vec2>&            points,
  const std::vector<SectorBuildData>& sectors,
  MapSectors&                         output,
  MapBuildFlags                       flags)
{
  using namespace MapBuildFlag;

  output.sectors.clear();
  output.walls.clear();
  output.portals.clear();

  // list of sectors for each point
  std::vector<std::vector<SectorID>> points_to_sectors;
  points_to_sectors.resize(points.size());

  // copy of the original sectors
  auto temp_sectors = sectors;

  [[maybe_unused]]const bool assert_on_check_fail = flags & assert_on_fail;

  const bool do_convexity_check = !(flags & omit_convexity_clockwise_check);
  const bool do_overlap_check   = !(flags & omit_sector_overlap_check);

  // Check for point and sector count
  u64 MAX_U16 = static_cast<u16>(-1);
  NC_ASSERT(points.size()  < MAX_U16);
  NC_ASSERT(sectors.size() < MAX_U16);

  // Check for non convex and non-clockwise order sectors
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

  // store which points are used by which sectors
  for (u64 i = 0; i < temp_sectors.size(); ++i)
  {
    for (auto&& wall : temp_sectors[i].points)
    {
      // TODO: can be kept sorted, so that later it takes
      // less time to find
      points_to_sectors[wall.point_index].push_back(static_cast<u16>(i));
    }
  }

  // find portal walls and store them
  for (u16 sector_id = 0; sector_id < temp_sectors.size(); ++sector_id)
  {
    auto&& sector = temp_sectors[sector_id];
    auto& output_sector = output.sectors.emplace_back();

    // copy the portable data
    output_sector.ext_data = sector.ext_data;

    const u32 total_wall_count = static_cast<u32>(sector.points.size());
    // we do not know this in advance, but
    // need if to calculate the offset into the ext_dataal array
    u32 total_portal_count = 0;   

    output_sector.int_data.first_wall = static_cast<WallID>(output.walls.size());
    output_sector.int_data.last_wall  = static_cast<WallID>(
      output_sector.int_data.first_wall + total_wall_count);

    output_sector.int_data.first_portal = static_cast<WallID>(output.portals.size());
    output_sector.int_data.last_portal  = output_sector.int_data.first_portal;

    // cycle through all walls and push them into the array
    for (u16 wall_index = 0; wall_index < sector.points.size(); ++wall_index)
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
        // TODO: the sequence can be kept sorted for faster search
        const bool whole_wall = std::find(b, e, used_sector_id) != e;
        if (whole_wall)
        {
          portal_with = used_sector_id;
          break;
        }
      }

      // Add a new wall
      output.walls.push_back(WallData
      {
        .pos              = points[point_index],
        .portal_sector_id = portal_with,
        .ext_data             = sector.points[wall_index].ext_data,
      });

      // And make it possibly a portal
      if (portal_with != INVALID_SECTOR_ID)
      {
        total_portal_count += 1;
        output.portals.push_back(WallPortalData
        {
          // is non-zero because we push a new wall above
          .wall_index = static_cast<WallID>(output.walls.size()-1),
        });
      }
    }

    output_sector.int_data.last_portal = static_cast<WallID>(
      output_sector.int_data.first_portal + total_portal_count);
  }

  // now, there should be the same number of sectors in the output as on the input
  NC_ASSERT(output.sectors.size() == sectors.size());

  // and finally, build the grid
  build_sector_grid(output);

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

  // and then the sectors
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

  // and then the sectors
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
  make_random_square_maze_map(map, 32, 0);

  std::vector<vec2> points_inside;

  while ((int)points_inside.size() < state.range(0))
  {
    f32 x = ((std::rand() % 4097) - 2048) / 2048.0f;
    f32 y = ((std::rand() % 4097) - 2048) / 2048.0f;
    if (map.get_sector_from_point(vec2{x, y}) != INVALID_SECTOR_ID)
    {
      points_inside.push_back(vec2{x, y});
    }
  }

  for (auto _ : state)
  {
    for (auto pt : points_inside)
    {
      auto viewpoint = Frustum2{.center = pt, .direction = vec2{1, 0}, .angle = Frustum2::FULL_ANGLE};
      SectorID last_sector = INVALID_SECTOR_ID;
      map.query_visible_sectors(viewpoint, [&](SectorID id, Frustum2)
      {
        last_sector = id;
      });
      benchmark::DoNotOptimize(last_sector);
    }

    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(points_inside.size() * state.iterations());
}

constexpr auto OMIT_CHECKS_FLAGS
  = map_building::MapBuildFlag::omit_convexity_clockwise_check
  | map_building::MapBuildFlag::omit_sector_overlap_check;

BENCHMARK_CAPTURE(benchmark_visibility_query,     "Visibility query")                             ->Arg(1<<12)->Unit(benchmark::kMillisecond);

BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (with checks)", 0)                ->Arg(16)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (no checks)",   OMIT_CHECKS_FLAGS)->Arg(16)->Unit(benchmark::kMillisecond);

BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (with checks)", 0)                ->Arg(32)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (no checks)",   OMIT_CHECKS_FLAGS)->Arg(32)->Unit(benchmark::kMillisecond);

BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (with checks)", 0)                ->Arg(48)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (no checks)",   OMIT_CHECKS_FLAGS)->Arg(48)->Unit(benchmark::kMillisecond);
#endif

}

