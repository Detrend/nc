// Project Nucledian Source File
#pragma once

#include <types.h>
#include <engine/map/map_types.h>

#include <map>
#include <functional>

namespace nc
{

struct MapSectors;

struct MapDynamics
{
  void update(f32 delta);

  struct SectorState
  {
    f32 floor_height;
    f32 ceil_height;
  };

  struct SectorEntry
  {
    std::vector<SectorState> states;
    f32                      speed      = 1.0f;
    u16                      prev_state = 0;
    u16                      state      = 0;
    bool                     rollback_on_entity_collision; // ?
  };

  using EntryMap       = std::map<SectorID, SectorEntry>;
  using SectorCallback = std::function<void(SectorID)>;

  MapSectors&    map;
  EntryMap       entries;
  SectorCallback callback;
};

}
