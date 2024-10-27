// Project Nucledian Source File
#pragma once

#include <types.h>
#include <vector_maths.h>
#include <intersect.h>

#include <vector>
#include <utility>
#include <float.h>
#include <algorithm>
#include <functional>

namespace nc
{
using SectorID  = u16;
using WallID    = u16;
using PortalID  = u16;
using TextureID = u16;

constexpr auto INVALID_SECTOR_ID  = static_cast<SectorID>(-1);
constexpr auto INVALID_WALL_ID    = static_cast<WallID>(-1);
constexpr auto INVALID_PORTAL_ID  = static_cast<PortalID>(-1);
constexpr auto INVALID_TEXTURE_ID = static_cast<TextureID>(-1);

struct SectorReprData
{
  // All indices are exclusive from top
  // To get number of walls in a sector you use
  // last_wall - first_wall
  // If first_wall == last_wall then the sector has no walls
  WallID   first_wall   = INVALID_WALL_ID; // [0..total_wall_count]
  WallID   last_wall    = INVALID_WALL_ID; // [first_wall+1..total_wall_count]
  PortalID first_portal = INVALID_WALL_ID; // [0..total_wall_count]
  PortalID last_portal  = INVALID_WALL_ID; // [first_portal..total_wall_count]
};

struct SectorPortableData
{
  TextureID floor_texture_id = INVALID_TEXTURE_ID;
  TextureID ceil_texture_id  = INVALID_TEXTURE_ID;
  f32       floor_height     = 0;
  f32       ceil_height      = 0;
};

struct SectorData
{
  SectorReprData     repr;
  SectorPortableData port;
};

struct WallPortableData
{
  TextureID texture_id;
  u8        texture_offset_x;
  u8        texture_offset_y;
};

struct WallData
{
  // The wall starts here and ends in the same point as the next
  // wall begins
  vec2             pos = vec2{0};
  SectorID         portal_sector_id = INVALID_SECTOR_ID;    // if is portal
  WallPortableData port;
};

struct WallPortalData
{
  WallID wall_index     = 0;
  // we are gonna need this for traversal of
  // literal portals
  u8     recursion_mark = 0;
};

struct MapSectors
{
  std::vector<SectorData>     sectors;
  std::vector<WallData>       walls;
  std::vector<WallPortalData> portals;

  using VisitorFunc = std::function<void(SectorID, Frustum2)>;
  void traverse_visible_areas(
    const Frustum2& input_frustum,
    VisitorFunc     visitor,
    u8              recursion_depth = 4) const;
};

namespace map_building
{

struct WallBuildData
{
  u16              point_index = 0;
  WallPortableData port;
};

struct SectorBuildData
{
  std::vector<WallBuildData> points;
  SectorPortableData         portable;
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

struct NonConvexInfo
{
  SectorID sector;
};

using MapBuildFlags = u8;
namespace MapBuildFlag
{
  enum etype : MapBuildFlags
  {
    omit_wall_overlap_check        = 1 << 0,
    omit_convexity_clockwise_check = 1 << 1,
    omit_sector_overlap_check      = 1 << 2,
  };
}

int build_map(
  const std::vector<vec2>&            points,
  const std::vector<SectorBuildData>& sectors,
  MapSectors&                         output,
  MapBuildFlags                       flags = 0);

}

}
