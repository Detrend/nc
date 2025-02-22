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
void for_each_portal(const MapSectors& map, SectorID sector_id, F&& lambda)
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
}

//==============================================================================
// TODO: an acceleration data structure is a MUST HAVE here!!!
SectorID get_sector_from_point(const MapSectors& map, vec2 point)
{
  for (SectorID sector_id = 0; sector_id < map.sectors.size(); ++sector_id)
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

      if (intersect::point_triangle(point, p1, p2, p3))
      {
        return sector_id;
      }
    }
  }

  return INVALID_SECTOR_ID;
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
void MapSectors::traverse_visible_areas(
  const Frustum2&    input_frustum,
  VisitorFunc        visitor,
  [[maybe_unused]]u8 max_recursion_depth) const
{
  NC_ASSERT(visitor);

  SectorID start_sector = map_helpers::get_sector_from_point(*this, input_frustum.center);

  if (start_sector == INVALID_SECTOR_ID)
  {
    // This point is outside of all sectors..
    return;
  }

  constexpr u64 FRUSTUM_CNT = 4;
  struct Frustums
  {
    std::array<Frustum2, FRUSTUM_CNT> frustums = {INVALID_FRUSTUM, INVALID_FRUSTUM, INVALID_FRUSTUM, INVALID_FRUSTUM};
    PortalID see_through_portal_id             = INVALID_PORTAL_ID;
  };

  std::map<SectorID, Frustums> curr_iteration;
  std::map<SectorID, Frustums> next_iteration;

  {
    Frustums to_insert{};
    to_insert.frustums[0] = input_frustum;
    next_iteration.insert({start_sector, to_insert});
  }

  // Now do a BFS
  while (next_iteration.size())
  {
    curr_iteration = std::move(next_iteration);
    NC_ASSERT(next_iteration.empty());

    for (const auto&[id, content] : curr_iteration)
    {
      NC_ASSERT(id < sectors.size());

      // Iterate all frustums in this dir
      for (const auto& frustum : content.frustums)
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
        map_helpers::for_each_portal(*this, id, [&](PortalID, WallID wall1_idx)
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
          if (next_iteration.contains(next_sector))
          {
            // Merge the two frustums if the sector is already in the queue..
            // Merge with overlapping one or insert it as a new one..
            auto& frustum_array = next_iteration[next_sector].frustums;
            u64   closest_idx   = 0;
            f32   closest_dst   = FLT_MAX;
            bool  merged        = false;

            for (u64 i = 0; i < FRUSTUM_CNT; ++i)
            {
              auto& other_frustum = frustum_array[i];
              auto  is_invalid    = other_frustum == INVALID_FRUSTUM;

              f32 angle_diff = is_invalid ? 0.0f : other_frustum.angle_difference(frustum);

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

              if (angle_diff < closest_dst)
              {
                angle_diff  = closest_dst;
                closest_idx = i;
              }
            }

            if (!merged)
            {
              // no overlapping frustum found? Then merge with a closest one
              NC_ASSERT(closest_idx < FRUSTUM_CNT);
              frustum_array[closest_idx].merged_with(new_frustum);
            }
          }
          else
          {
            Frustums to_insert{};
            to_insert.frustums[0] = new_frustum;
            next_iteration.insert({next_sector, to_insert});
          }
        });
      }

    }
  }
}

}

namespace nc::map_building
{

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
  vec2 map_min = vec2{FLT_MAX, FLT_MAX};
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
    grid.query_aabb(bbox, [&](aabb2, SectorID sector)
    {
      if (sector != sector_idx)
      {
        possible_intersection_sectors.insert(sector);
      }
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
      u16 i1 = i * SIZE + j;
      u16 i2 = i1+1;
      u16 i3 = (i+1) * SIZE + j + 1;
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

constexpr auto OMIT_CHECKS_FLAGS
  = map_building::MapBuildFlag::omit_convexity_clockwise_check
  | map_building::MapBuildFlag::omit_sector_overlap_check;

BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (with checks)", 0)                ->Arg(16)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (no checks)",   OMIT_CHECKS_FLAGS)->Arg(16)->Unit(benchmark::kMillisecond);

BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (with checks)", 0)                ->Arg(32)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (no checks)",   OMIT_CHECKS_FLAGS)->Arg(32)->Unit(benchmark::kMillisecond);

BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (with checks)", 0)                ->Arg(48)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(benchmark_map_creation_squared, "Map building (no checks)",   OMIT_CHECKS_FLAGS)->Arg(48)->Unit(benchmark::kMillisecond);
#endif

}

