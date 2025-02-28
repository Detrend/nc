// Project Nucledian Source File
#pragma once

// MR says:
// This is a primary data representation of the map during runtime
// of the game. It is supposed to be fully static (no adding/removing
// of sectors during runtime).
// The map is composed of sectors, where each sector is a convex 2D
// shape surrounded by walls. The ordering of the walls is in
// counter-clockwise order.
// Two sectors can be connected together by so-called "portal",
// which acts as an edge on the graph whose vertices are the
// individual sectors.
// The purpose of this data representation is a fast traversal and
// visibility culling. Therefore, the main use is in rendering and
// enemy-player visibility checking.
// The data of walls and sectors is tightly packed together in an
// array for cache-friendly traversal.
// It should be possible to fully derive the level geometry from
// this data representation.

// INTERNAL vs EXTERNAL data:
// Several parts of this code work with internal/external sector/wall
// data.
// Internal data is a set of data generated during building of the
// map from other type of data. Do not touch internal data.
// External data is a set of data associated with walls/sectors
// that can be shared with other map representation (editor for
// example). These are never changed during building of the sector.
// If you want to associate some data with each sector/wall, then
// put it into external data.

#include <types.h>
#include <vector_maths.h>
#include <intersect.h>
#include <grid.h>

#include <vector>
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

// Portable data are a set of data that can be shared among two different
// map representations. Put here anything that you want in sectors
struct SectorExtData
{
  TextureID floor_texture_id = INVALID_TEXTURE_ID;
  TextureID ceil_texture_id  = INVALID_TEXTURE_ID;
  f32       floor_height     = 0;
  f32       ceil_height      = 0;
};

// Put here anything you want in each wall
struct WallExtData
{
  TextureID texture_id;
  u8        texture_offset_x;
  u8        texture_offset_y;
};

// Internal data of this map representation.
struct SectorIntData
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

// Each sector is comprised of internal data (data 
struct SectorData
{
  SectorIntData int_data;
  SectorExtData ext_data;
};

struct WallData
{
  // The wall starts here and ends in the same point as the next
  // wall begins
  vec2        pos = vec2{0};
  SectorID    portal_sector_id = INVALID_SECTOR_ID;    // if is portal
  WallExtData ext_data;
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
  StatGridAABB2<SectorID>     sector_grid;

  using TraverseVisitor = std::function<void(SectorID, Frustum2)>;
  using PortalVisitor   = std::function<void(PortalID, WallID)>;
  // Traverses the sector system in a BFS order and calls the visitor
  // for each sector with a frustum that describes which parts of the
  // sector are visible.
  void query_visible_sectors(Frustum2 frustum, TraverseVisitor visitor) const;

  // Iterates all portals of this sector
  bool for_each_portal_of_sector(SectorID sector, PortalVisitor visitor) const;

  // Returns an id of a sector that lies on this position. If there is
  // no such sector then returns INVALID_SECTOR_ID
  SectorID get_sector_from_point(vec2 point) const;
};

namespace map_building
{

struct WallBuildData
{
  u16         point_index = 0;
  WallExtData ext_data;
};

struct SectorBuildData
{
  std::vector<WallBuildData> points;
  SectorExtData              ext_data;
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
    omit_convexity_clockwise_check = 1 << 0,
    omit_sector_overlap_check      = 1 << 1,
    assert_on_fail                 = 1 << 2,
  };
}

int build_map(
  const std::vector<vec2>&            points,
  const std::vector<SectorBuildData>& sectors,
  MapSectors&                         output,
  MapBuildFlags                       flags = 0);

}

}
