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
  OverlapInfo&                        overlap);
  
inline int build_map(
  const std::vector<vec2>&            points,
  const std::vector<SectorBuildData>& sectors,
  MapSectors&                         output);

}

}
