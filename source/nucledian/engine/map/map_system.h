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

#include <engine/map/map_types.h>

#include <types.h>
#include <math/vector.h>
#include <math/matrix.h>
#include <intersect.h>
#include <grid.h>

#include <vector>
#include <functional>

namespace nc
{

// Internal data of this map representation.
struct SectorIntData
{
  // All indices are exclusive from top
  // To get number of walls in a sector you use
  // last_wall - first_wall
  // If first_wall == last_wall then the sector has no walls
  WallID   first_wall = INVALID_WALL_ID; // [0..total_wall_count]
  WallID   last_wall  = INVALID_WALL_ID; // [first_wall..total_wall_count]
};

// Each sector is comprised of internal data
struct SectorData
{
  SectorIntData int_data;
};

using PortType = u8;
namespace PortalType
{
  enum evalue : PortType
  {
    none = 0,      // a wall
    classic,       // normal portal between two neighboring sectors
    non_euclidean, // non euclidean portal
  };
}

struct WallData
{
  // The wall starts here and ends in the same point as the next
  // wall begins
  vec2      pos = vec2{0};
  SectorID  portal_sector_id  = INVALID_SECTOR_ID; // if is portal
  WallRelID nc_portal_wall_id = INVALID_WALL_REL_ID;

  PortType get_portal_type() const;
};

// TODO: maybe organize each data type into separate table row instead of grouping them up?
// TODO: the complexity of query algorithm can increase quite a lot in certain situations..
// Doing a Dijkstra instead of traditional BFS might fix this.
struct MapSectors
{
  template<typename T>
  using column = std::vector<T>;

  column<SectorData>      sectors;
  column<WallData>        walls;
  StatGridAABB2<SectorID> sector_grid;

  // TODO: replace these later once we can do sectors with varying height
  static constexpr f32 SECTOR_FLOOR_Y   = 0.0f;
  static constexpr f32 SECTOR_CEILING_Y = 1.5f;

  // TODO: do not use the retarded std::function, find a better alternative
  using TraverseVisitor = std::function<void(SectorID, Frustum2, WallID)>;
  using WallVisitor     = std::function<void(WallID)>;
  // Traverses the sector system in a BFS order and calls the visitor
  // for each sector with a frustum that describes which parts of the
  // sector are visible.
  void query_visible_sectors(
    vec2            position,    // exact position on the map
    vec2            view_dir,    // normalized view direction
    f32             hor_fov_rad, // [0-Pi], in radians. >= Pi means 360 degrees of view
    TraverseVisitor visitor) const;    // callback function that is called for each visited sector

  // Iterates all portals of this sector
  bool for_each_portal_of_sector(SectorID sector, WallVisitor visitor) const;

  // Returns an id of a sector that lies on this position. If there is
  // no such sector then returns INVALID_SECTOR_ID. If there are multiple
  // sectors covering this point (on sector edges) then one of them is
  // returned.
  SectorID get_sector_from_point(vec2 point) const;

  mat4 calculate_portal_to_portal_projection(
    SectorID from_sector,
    WallID   from_portal) const;

  void sector_to_vertices(
    SectorID           sector_id,
    std::vector<vec3>& vertices_out) const;

  struct PortalSector
  {
    WallID   wall_id;
    SectorID sector_id;
  };
  using Portals = std::vector<PortalSector>;

  bool raycast2d_expanded(
    vec2     from,
    vec2     to,
    f32      expand,
    vec2&    out_normal,
    f32&     out_coeff,
    Portals* out_portals = nullptr) const;

  bool raycast3d(vec3 from, vec3 to, vec3& out_normal, f32& out_coeff) const;

private:
  void query_visible_sectors_impl(
    const SectorID*      start_sectors,
    u32                  start_sector_cnt,
    const FrustumBuffer& frustum,
    TraverseVisitor      visitor,
    u8                   recursion_depth = 4,
    WallID               source_portal   = INVALID_WALL_ID) const;
};

namespace map_building
{

struct WallBuildData
{
  WallID    point_index = 0;
  WallRelID nc_portal_point_index  = 0;
  SectorID  nc_portal_sector_index = INVALID_SECTOR_ID;
};

struct SectorBuildData
{
  std::vector<WallBuildData> points;
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
