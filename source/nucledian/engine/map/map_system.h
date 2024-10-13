// Project Nucledian Source File
#pragma once

#include <types.h>
#include <vector_maths.h>
#include <intersect.h>

#include <vector>
#include <utility>
#include <float.h>
#include <algorithm>

namespace nc
{
using SectorID = u16;
using WallID   = u16;
using PortalID = u16;

struct SectorReprData
{
  WallID   first_wall;
  WallID   last_wall;
  PortalID first_portal;
  PortalID last_portal;

  f32 floor_height1;      // at index 0
  f32 floor_height2;      // at index floor_height2_index

  f32 ceil_height1;       // at index 0
  f32 ceil_height2;       // at index ceil_height2_index

  u16 floor_height2_index;
  u16 ceil_height2_index;
};

struct SectorPortableData
{
  u32 floor_texture_id;
  u32 ceil_texture_id;
};

struct SectorData
{
  SectorReprData     repr;
  SectorPortableData port;
};

struct WallPortableData
{
  u32  texture_id;
  u8   texture_offset_x;
  u8   texture_offset_y;
};

struct WallData
{
  vec2 pos;
  u16  portal_sector_id;    // if is portal
  WallPortableData port;
};

struct WallPortalData
{
  WallID wall_index;
};

struct MapSectors
{
  std::vector<SectorData>     sectors;
  std::vector<WallData>       walls;
  std::vector<WallPortalData> portals;
};

namespace map_building
{

struct WallBuildData
{
  u32 texture_id;
  u16 point_index;
  u8  tex_offset_x;
  u8  tex_offset_y;
  WallPortableData port;
};

struct SectorBuildData
{
  std::vector<WallBuildData> points;
  SectorPortableData         portable;

  vec3                       floor_point1;
  vec3                       floor_point2;

  vec3                       ceil_point1;
  vec3                       ceil_point2;
};

struct OverlapInfo
{
  u16  sector1;
  u16  sector2;
  u16  wall1;
  u16  wall2;
  u16  w1p1;
  u16  w1p2;
  u16  w2p1;
  u16  w2p2;
  vec2 p;
};

// a stupid algoritm (TODO: make smarter) for wall overlap check
inline bool check_for_wall_overlaps(
  const std::vector<vec2>&            points,
  const std::vector<SectorBuildData>& sectors,
  OverlapInfo&                        overlap)
{
  std::vector<aabb2> sector_bboxes;
  sector_bboxes.resize(sectors.size());
  
  // create the bboxes
  for (u64 sector_idx = 0; sector_idx < sectors.size(); ++sector_idx)
  {
    auto&& sector = sectors[sector_idx];
    for (u64 point_idx = 0; point_idx < sector.points.size(); ++point_idx)
    {
      auto&& point = points[point_idx];
      sector_bboxes[sector_idx].insert_point(point);
    }
  }

  // now iterate all sectors
  for (u16 sector1_idx = 0; sector1_idx < sectors.size(); ++sector1_idx)
  {
    for (u16 sector2_idx = sector1_idx+1; sector2_idx < sectors.size(); ++sector2_idx)
    {
      auto&& sector1_bbox = sector_bboxes[sector1_idx];
      auto&& sector2_bbox = sector_bboxes[sector2_idx];

      // quick exit if the bboxes do not overlap
      if (!intersect::aabb_aabb_2d(sector1_bbox, sector2_bbox))
      {
        continue;
      }

      // now intersect all the walls
      auto&& sector1 = sectors[sector1_idx];
      auto&& sector2 = sectors[sector2_idx];

      // for all walls of sector1
      {
        for (u16 s1p1_idx = 0; s1p1_idx < sector1.points.size(); ++s1p1_idx)
        {
          u16 s1p2_idx = (s1p1_idx+1) % sector1.points.size();

          for (u16 s2p1_idx = 0; s2p1_idx < sector2.points.size(); ++s2p1_idx)
          {
            u16 s2p2_idx = (s2p1_idx+1) % sector2.points.size();

            const u16 s1p1_wi = sector1.points[s1p1_idx].point_index;
            const u16 s1p2_wi = sector1.points[s1p2_idx].point_index;
            const u16 s2p1_wi = sector2.points[s2p1_idx].point_index;
            const u16 s2p2_wi = sector2.points[s2p2_idx].point_index;

            if (s1p1_wi == s2p1_wi || s1p1_wi == s2p2_wi
             || s1p2_wi == s2p1_wi || s1p2_wi == s2p2_wi)
            {
              continue;
            }

            const vec2 s1p1 = points[s1p1_wi];
            const vec2 s1p2 = points[s1p2_wi];
            const vec2 s2p1 = points[s2p1_wi];
            const vec2 s2p2 = points[s2p2_wi];

            const aabb2 bbox1{s1p1, s1p2};
            const aabb2 bbox2{s2p1, s2p2};

            if (!intersect::aabb_aabb_2d(bbox1, bbox2))
            {
              continue;
            }

            vec2 out;
            if (!intersect::segment_segment_2d(s1p1, s1p2, s2p1, s2p2, out))
            {
              continue;
            }

            overlap = OverlapInfo
            {
              .sector1 = sector1_idx,
              .sector2 = sector2_idx,
              .wall1 = s1p1_idx,
              .wall2 = s2p1_idx,
              .w1p1 = s1p1_wi,
              .w1p2 = s1p2_wi,
              .w2p1 = s2p1_wi,
              .w2p2 = s2p2_wi,
            };
            return false;
          }
        }
      }
    }
  }

  return true;
}
  
inline int build_map(
  const std::vector<vec2>&            points,
  const std::vector<SectorBuildData>& sectors,
  MapSectors&                         output)
{
  output.sectors.clear();
  output.walls.clear();
  output.portals.clear();

  vec2 map_min = vec2{FLT_MAX, FLT_MAX};
  vec2 map_max = vec2{FLT_MIN, FLT_MIN};

  // find map minimum and maximum
  for (auto&& point : points)
  {
    map_min = min(map_min, point);
    map_max = max(map_max, point);
  }

  // list of sectors for each point
  std::vector<std::vector<u16>> points_to_sectors;
  points_to_sectors.resize(points.size());

  // copy of the original sectors
  auto temp_sectors = sectors;

  // Check for wall overlaps (not allowed)
  if (OverlapInfo oi; !check_for_wall_overlaps(points, sectors, oi))
  {
    return false;
  }

  // TODO: resolve non-convex sectors
  // - first, we would have to store them into a grid

  // store which points are used by which sectors
  for (u64 i = 0; i < temp_sectors.size(); ++i)
  {
    for (auto&& wall : temp_sectors[i].points)
    {
      // TODO: can be kept sorted, so that later it takes
      // less time to find
      points_to_sectors[wall.point_index].push_back(i);
    }
  }

  // find portal walls and store them
  for (u16 sector_id = 0; sector_id < temp_sectors.size(); ++sector_id)
  {
    auto&& sector = temp_sectors[sector_id];

    struct PortalWall
    {
      u16 wall_index;
      u16 other_sector;
    };
    std::vector<PortalWall> portal_walls;

    // cycle through all walls
    for (u16 wall_index = 0; wall_index < sector.points.size(); ++wall_index)
    {
      u16 next_wall_index = (wall_index + 1) % sector.points.size();
      u16 point_index = sector.points[wall_index].point_index;
      u16 next_point_index = sector.points[next_wall_index].point_index;

      auto& neighbor_sectors_of_p1 = points_to_sectors[point_index];
      for (u16 used_sector_id : neighbor_sectors_of_p1)
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
          portal_walls.push_back(PortalWall
          {
            .wall_index   = wall_index,
            .other_sector = used_sector_id,
          });
        }
      }
    }

    // now we should have enough info about all walls and should be able to build the sector
  }
}

}

}
