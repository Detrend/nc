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

struct TriggerData
{
  enum TriggerType : u8
  {
    sector,
    wall,
    entity,
  };

  ActivatorID activator = INVALID_ACTIVATOR_ID; // Activator to activate

  // Resets back to off after the time
  f32  timeout = 0.0f;

  // Can be turned off manually? By leaving (sector) or triggering again (wall)
  bool can_turn_off     : 1 = false; // Can be turned back off manually
  bool player_sensitive : 1 = true;  // Triggers by player
  bool enemy_sensitive  : 1 = false; // Triggers by enemy
  bool while_alive      : 1 = false; // False = active while dead, true = while alive
  u8   type             : 2 = TriggerType::sector;

  union
  {
    struct
    {
      SectorID sector;
    } sector_type;

    struct
    {
      SectorID  sector;
      WallRelID wall;
      u8        segment;
    } wall_type;

    struct 
    {
      EntityID entity;
    } entity_type;
  };

  bool has_timeout() const
  {
    return timeout > 0.0f;
  }
};

struct ActivatorData
{
  u16 threshold = 0; // How many triggers have to be activated to turn on
  // TODO: Some other properties..
  // Like if it should print some text on the screen..
};

struct MapDynamics
{
  MapDynamics(MapSectors& map, EntityRegistry& registry, SectorMapping& mapping);

  void on_map_rebuild_and_entities_created();

  void update(f32 delta);

  void evaluate_activators(std::vector<u16>& out_values, f32 update_dt = 0.0f);

  void switch_wall_segment_trigger(SectorID sector, WallID wall, u8 segment);

  struct RuntimeSegmentInfo
  {
    f32       countdown = 0.0f;
    bool      triggered = false;
    TriggerID trigger   = INVALID_TRIGGER_ID;
  };

  using SectorChangeCallback = std::function<void(SectorID)>;
  using ActivatorList        = std::vector<std::vector<SectorID>>;
  using SegmentRuntimeMap    = std::unordered_map<u32, RuntimeSegmentInfo>;

  // Bindings to the remaining systems
  MapSectors&     map;
  EntityRegistry& registry;
  SectorMapping&  mapping;

  // This has to be saved and loaded from the savefile
  SegmentRuntimeMap segment_trigger_runtime;

  // Set these before calling on_map_rebuild_and_entities_created
  // No need to save/load
  std::vector<TriggerData>   triggers;
  std::vector<ActivatorData> activators;

  SectorChangeCallback  sector_change_callback;
  ActivatorList         activator_list;
};

}
