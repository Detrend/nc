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
#include <engine/graphics/texture_id.h>

#include <vector>
#include <functional>

namespace nc
{

// This data structure lets us check which sectors and their parts are visible
// and which are not.
// Can be used for occlusion culling or detecting visible regions by AI
struct VisibilityTree
{
  struct SectorFrustum
  {
    SectorID      sector;  // part of this sector is visible
    FrustumBuffer frustum; // and this is that part
  };

  // This is the portal we see into the sector through. If invalid
  // then it means that we are observing the sector straight
  // through our eyes and not through a portal
  WallID                      portal_wall   = INVALID_WALL_ID;
  SectorID                    portal_sector = INVALID_SECTOR_ID;
  // This is a list of sectors that are visible
  std::vector<SectorFrustum>  sectors;
  // And this is a list of portals we see (might be empty)
  std::vector<VisibilityTree> children;

  // Runs through the tree and checks if the given sector is visible.
  // If visible then also outputs the smallest depth at which this
  // sector is visible
  bool is_visible(SectorID id, u64& depth) const;

  // Same as above but without outputting the depth
  bool is_visible(SectorID id) const;
};

struct SectorSet
{
  std::vector<SectorID> sectors;
  std::vector<mat4>     transforms;
};

// Internal data of this map representation.
struct SectorIntData
{
  // All indices are exclusive from top
  // To get number of walls in a sector you use
  // last_wall - first_wall
  // If first_wall == last_wall then the sector has no walls
  WallID first_wall = INVALID_WALL_ID; // [0..total_wall_count]
  WallID last_wall  = INVALID_WALL_ID; // [first_wall..total_wall_count]
};

// Describes how a surface should be rendered.
struct SurfaceData
{
  TextureID texture_id = INVALID_TEXTURE_ID;
  f32       scale      = 1.0f;
  // Rotation in radians, counter-clockwise.
  f32       rotation   = 0.0f;
  // Offset in texture coordinates [0-1].
  vec2      offset     = VEC2_ZERO;
  // How many different ways a texture tile is allowed to be randomly rotated
  u32       tile_rotations_count = 1;
  // Fixed step between the randomized tile rotation options (in radians)
  float     tile_rotation_increment = 1.5707963267948966f;
  // If false, skip generating the mesh entirely
  bool      should_show = true;
};

// Describes how a wall surface should be rendered.
// Wall can be divided into multiple height intervals, which each has a different texture
struct WallSurfaceData {
    enum Flags : u32 {
        none = 0,
        generate_left_face = 1,
        generate_right_face = 2,
        generate_up_face = 4,
        generate_down_face = 8,
        generate_all_faces = 0xF,
        flip_side_normals = 16,
        absolute_directions = 32
    };
    struct Entry {
        // Surface used in this wall interval
        SurfaceData surface;
        // Height in absolute world coords, where this segment ends. It begins at the end of the previous entry
        f32         end_height = +INFINITY;
        vec3        end_up_tesselation  = vec3(0.0f, 0.0f, 0.0f);
        vec3        end_down_tesselation  = vec3(0.0f, 0.0f, 0.0f);
        vec3        begin_up_tesselation = vec3(0.0f, 0.0f, 0.0f);
        vec3        begin_down_tesselation = vec3(0.0f, 0.0f, 0.0f);
        Flags       flags = Flags::generate_all_faces;
    };

    // Height intervals either for the floor-difference part of the wall (if this wall has a portal connecting two sectors), or for the whole wall (if there are no two neighbors)
    std::vector<Entry> surfaces;
};

// Each sector is comprised of internal data
struct SectorData
{
  SectorIntData int_data;
  f32           floor_height  = 0.0f;
  f32           ceil_height   = 0.0f;
  SurfaceData   floor_surface;
  SurfaceData   ceil_surface;
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
  
  vec2           pos               = vec2{0};
  SectorID       portal_sector_id  = INVALID_SECTOR_ID; // if is portal
  WallRelID      nc_portal_wall_id = INVALID_WALL_REL_ID;
  PortalRenderID render_data_index = INVALID_PORTAL_RENDER_ID;
  WallSurfaceData surface;

  PortType get_portal_type() const;

  // Returns true if this is a normal portal or nuclidean portal.
  // Returns false if this is a wall
  bool is_portal() const;
};

struct Portal
{
  // Rotation along the Y-axis.
  const f32  rotation    = 0.0f;
  const vec3 position    = VEC3_ZERO;
  // Local to world.
  const mat4 transform   = mat4(1.0f);
  /*
  * Compute the view matrix for the virtual camera looking through the destination portal.
  * The camera’s relative offset to the source portal is preserved when projecting through.
  *
  * First, build the virtual camera’s world transform by:
  *   1. Transforming the camera from local to world space.
  *   2. Converting from world space into the source portal’s local space.
  *   3. Rotating 180° around the up (Y) axis.
  *   4. Transforming into the destination portal’s world space.
  *
  * To get the view matrix (i.e., the inverse world-to-local transform), apply the inverse sequence:
  *   1. Destination portal: world-to-local.
  *   2. Rotate 180° around the up (Y) axis (inverse of 180° is 180°).
  *   3. Source portal: local-to-world.
  *   4. Camera: world-to-local.
  *
  * dest_to_src represent steps 1. - 3.
  */
  const mat4 dest_to_src = mat4(1.0f);
};

// TODO: maybe organize each data type into separate table row instead of grouping them up?
// TODO: the complexity of query algorithm can increase quite a lot in certain situations..
// Doing a Dijkstra instead of traditional BFS might fix this.
struct MapSectors
{
  template<typename T>
  using column = std::vector<T>;

  column<SectorData>       sectors;
  column<WallData>         walls;
  column<Portal>           portals_render_data;
  column<aabb3>            sector_bboxes;
  StatGridAABB2<SectorID>  sector_grid;

  // TODO: replace these later once we can do sectors with varying height
  static constexpr f32 SECTOR_FLOOR_Y   = 0.0f;
  static constexpr f32 SECTOR_CEILING_Y = 1.5f;

  // TODO: do not use the retarded std::function, find a better alternative
  using TraverseVisitor = std::function<void(SectorID, Frustum2, WallID)>;
  using WallVisitor     = std::function<void(WallID)>;
  // Traverses the sector system in a BFS order and calls the visitor
  // for each sector with a frustum that describes which parts of the
  // sector are visible.
  void query_visible
  (
    vec3            position,        // exact position on the map
    vec3            view_dir,        // normalized view direction
    f32             hor_fov_rad,     // [0-Pi], in radians. >= Pi means 360 degrees of view
    f32             ver_fov_rad,     // Vertical FOV in radians, [0-Pi)
    VisibilityTree& visibility_tree, // callback function that is called for each visited sector
    u8              recursion_depth  // depth 0 means only the current "dimension" without any traversal of portals
  ) const;

  // Queries a sectors nearby a point. This includes sectors behind nuclidean
  // portals. Implemented as a floodfill.
  // Returns a set of sectors and their transforms relative to the point.
  // Optimized for small ranges.
  // This is a very specific function for very specific cases and not for a
  // general use.
  void query_nearby_sectors_short_distance
  (
    vec2       position,
    f32        range,
    SectorSet& sectors_out
  )
  const;

  // Iterates all portals of this sector
  bool for_each_portal_of_sector(SectorID sector, WallVisitor visitor) const;

  // Returns an id of a sector that lies on this position. If there is
  // no such sector then returns INVALID_SECTOR_ID. If there are multiple
  // sectors covering this point (on sector edges) then one of them is
  // returned.
  SectorID get_sector_from_point(vec2 point) const;

  mat4 calc_portal_to_portal_projection(
    SectorID from_sector,
    WallID   from_portal) const;

  void sector_to_vertices(
    SectorID           sector_id,
    std::vector<f32>& vertices_out) const;

  std::vector<vec3> get_path(vec3 start_pos, vec3 end_pos, f32 width, f32 height) const;
  bool is_point_in_sector(vec2 pt, SectorID sector)      const;
  f32  distance_from_sector_2d(vec2 pt, SectorID sector) const;

  bool is_valid_sector_id(SectorID id) const;
  bool is_valid_wall_id(WallID id)     const;

private:
  void query_visible_sectors_impl(
    const SectorID*      start_sectors,
    u32                  start_sector_cnt,
    const FrustumBuffer& frustum,
    VisibilityTree&      visible,
    u8                   recursion_depth) const;
};

namespace map_building
{

struct WallBuildData
{
  WallID      point_index = 0;
  WallRelID   nc_portal_point_index  = 0;
  SectorID    nc_portal_sector_index = INVALID_SECTOR_ID;
  WallSurfaceData surface;
};

struct SectorBuildData
{
  std::vector<WallBuildData> points;
  f32                        floor_y;
  f32                        ceil_y;
  SurfaceData                floor_surface;
  SurfaceData                ceil_surface;
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

int build_map
(
  const std::vector<vec2>&            points,
  const std::vector<SectorBuildData>& sectors,
  MapSectors&                         output,
  MapBuildFlags                       flags = 0
);

}

namespace map_helpers
{

WallID next_wall(const MapSectors& map, SectorID sector, WallID wall);

WallID get_wall_id(const MapSectors& map, SectorID sector_id, WallRelID relative_wall_id);

WallID get_nc_opposing_wall(const MapSectors& map, SectorID sid, WallID wid);

}

}
