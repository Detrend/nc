// Project Nucledian Source File
#pragma once

#include <types.h>
#include <engine/map/map_types.h>
#include <engine/entity/entity_types.h>

#include <map>
#include <functional>
#include <string>

//                       .------------------------.                           //
//                       |        TRIGGERS        |                           //
//     .-----------------X------------------------X---------------------.     //
//     |      TYPE       |     ACTIVATED WHEN     | ACTIVATED ONLY ONCE |     //
//     |-----------------X------------------------X---------------------|     //
//     | Segment of wall | Interacts with segment |          NO         |     //
//     | On the sector   | Player enters sector   |          NO         |     //
//     | On the enemy    | Enemy dies             |         YES         |     //
//     |----------------------------------------------------------------|     //

namespace nc
{

struct MapSectors;
class  EntityRegistry;
struct SectorMapping;

struct MapDynamics
{
  MapDynamics(MapSectors& map, EntityRegistry& registry, SectorMapping& mapping);

  void on_map_rebuild();

  void update(f32 delta);

  void switch_wall_segment_trigger(SectorID sector, WallID wall, u16 segment);

  struct Activator
  {
    u16 counter = 0; // Number of enemies that died
  };

  struct ActivatorLookData
  {
    struct SegmentInfo
    {
      f32      countdown   = 0.0f;
      SectorID sector      = INVALID_SECTOR_ID;
      WallID   wall        = INVALID_WALL_ID;
      u8       segment_idx = 0;
      bool     triggered   = false;
    };

    std::vector<EntityID>    entity_watch_list;
    std::vector<SegmentInfo> segment_watch_list;
    std::vector<SectorID>    sector_watch_list;
    std::vector<SectorID>    influenced_sectors;
  };

  using SectorCallback = std::function<void(SectorID)>;
  using ActivatorList  = std::map<ActivatorID, ActivatorLookData>;

  MapSectors&     map;
  EntityRegistry& registry;
  SectorMapping&  mapping;
  SectorCallback  callback;
  ActivatorList   activator_to_sector_map;
};

}
