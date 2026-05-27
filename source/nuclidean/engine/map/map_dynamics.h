// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <engine/map/map_types.h>
#include <engine/entity/entity_types.h>

#include <map>
#include <functional>
#include <string>

#include <json/json_fwd.hpp>

//                       .------------------------.                           //
//                       |        TRIGGERS        |                           //
//     .-----------------X------------------------X---------------------.     //
//     |      TYPE       |     ACTIVATED WHEN     | ACTIVATED ONLY ONCE |     //
//     |-----------------X------------------------X---------------------|     //
//     | Segment of wall | Interacts with segment |          NO         |     //
//     | On the sector   | Player enters sector   |          NO         |     //
//     | On the entity   | Entity dies            |         YES         |     //
//     |----------------------------------------------------------------|     //

namespace nc
{

struct MapSectors;
class  EntityRegistry;
struct SectorMapping;
class  Buffer;

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
  s8   increment            = 1;     // How much should be activator value incremented
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

struct ActivatorHookLoadArg {

  ActivatorHookLoadArg(const nlohmann::json* the_hook_data, const std::unordered_map<unsigned, EntityID>* the_tags_to_entities, const std::unordered_map<unsigned, const nlohmann::json*>* the_tags_to_rawdata)
    : _hook_data(the_hook_data), _tags_to_entities(the_tags_to_entities), _tags_to_rawdata(the_tags_to_rawdata) {}

  const nlohmann::json& data() const { return *_hook_data; }
  EntityID get_entity(const unsigned tag) const { return (*_tags_to_entities).find(tag)->second; }
  const nlohmann::json& get_additional_data(const unsigned tag) const { return *(*_tags_to_rawdata).find(tag)->second; }

private:
  const nlohmann::json *_hook_data;
  const std::unordered_map<unsigned, EntityID> *_tags_to_entities;
  const std::unordered_map<unsigned, const nlohmann::json*> *_tags_to_rawdata;
};

struct ActivatorHookArg {
  f32 delta;
  std::vector<EntityID> entities;
};
class IActivatorHook
{
public:
  
  virtual ~IActivatorHook(){}

  virtual void load(const ActivatorHookLoadArg& arg) = 0;
  virtual void on_activated_update([[maybe_unused]]const ActivatorHookArg & args) {}
  virtual void on_activated_start([[maybe_unused]] const ActivatorHookArg& args){}
  virtual void on_activated_end([[maybe_unused]] const ActivatorHookArg& args){}
};


struct ActivatorData
{

  // How many triggers have to be activated to turn on
  u16                                               threshold = 0; 
  // Sectors whose floor/ceiling can dynamically change based on the activator's state
  std::vector<SectorID>                             affected_sectors;
  // Generic behaviors that happen depending on the activator
  std::vector<std::unique_ptr<IActivatorHook>>      hooks;


  // TODO: Some other properties..
  // Like if it should print some text on the screen..
};  

struct MapDynamics
{
  MapDynamics(MapSectors& map, EntityRegistry& registry, SectorMapping& mapping);

  void on_map_rebuild_and_entities_created();

  void on_destroy();

  void update(f32 delta);

  void evaluate_activators
  (
    std::vector<u16>& out_values, f32 update_dt = 0.0f, bool notify = false, std::vector<ActivatorHookArg> *const out_info = nullptr
  );

  bool switch_wall_segment_trigger(SectorID sector, WallID wall, u8 segment, bool& turned_on);

  SegmentID segment_id_from_trigger(const TriggerData& td) const;

  void on_sector_moving_changed(SectorID sector, bool started_moving, bool moving_up);

  void serialize(Buffer& buffer);

  using SectorChangeCallback = std::function<void(SectorID)>;
  using ActivatorList        = std::vector<std::vector<SectorID>>;
  using SegmentRuntimeMap    = std::unordered_map<u32, TriggerID>;

  // Bindings to the remaining systems
  MapSectors&     map;
  EntityRegistry& registry;
  SectorMapping&  mapping;

  // Set these before calling on_map_rebuild_and_entities_created
  // No need to save/load
  std::vector<TriggerData>   triggers;
  std::vector<ActivatorData> activators;
  std::vector<u8>            activators_dynamic;
  std::vector<EntityID>      sector_sounds; // Active sounds for moving sectors
  std::vector<bool>          moving_sectors;

  SectorChangeCallback  sector_change_callback;
};

}
