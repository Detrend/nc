// Project Nucledian Source File

#include <engine/map/map_dynamics.h>
#include <engine/map/map_system.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_types.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>

#include <math/lingebra.h>
#include <math/utils.h>

#include <algorithm> // std::count_if


namespace nc
{

//==============================================================================
static u32 sector_wall_segment_to_u32(SectorID sid, WallRelID wrelid, u8 segment)
{
  static_assert(sizeof(sid)     == 2);
  static_assert(sizeof(wrelid)  == 1);
  static_assert(sizeof(segment) == 1);

  return (u32{sid} << 16) | (u32{wrelid} << 8) | segment;
}

//==============================================================================
MapDynamics::MapDynamics
(
  MapSectors&     the_map,
  EntityRegistry& the_registry,
  SectorMapping&  the_mapping)
: map(the_map)
, registry(the_registry)
, mapping(the_mapping)
{

}

//==============================================================================
void MapDynamics::on_map_rebuild_and_entities_created()
{
  nc_assert(triggers.size() <= MAX_TRIGGERS);
  TriggerID trigger_cnt = cast<TriggerID>(triggers.size());

  // Create a mapping of runtime trigger states
  for (TriggerID tid = 0; tid < trigger_cnt; ++tid)
  {
    const TriggerData& td = triggers[tid];
    if (!td.type == TriggerData::wall)
    {
      continue;
    }

    u32 idx = sector_wall_segment_to_u32
    (
      td.wall_type.sector, td.wall_type.wall, td.wall_type.segment
    );

    nc_assert(!segment_trigger_runtime.contains(idx));
    segment_trigger_runtime.insert({idx, RuntimeSegmentInfo{}});
  }

  // Map sectors to their activators
  activator_list.resize(activators.size());

  nc_assert(map.sectors.size() < MAX_SECTORS);
  SectorID sector_cnt = cast<SectorID>(map.sectors.size());
  for (SectorID sid = 0; sid < sector_cnt; ++sid)
  {
    const SectorData& sd = map.sectors[sid];

    if (sd.activator != INVALID_ACTIVATOR_ID)
    {
      nc_assert(sd.activator < activator_list.size());
      activator_list[sd.activator].push_back(sid);
    }
  }
}

//==============================================================================
void MapDynamics::switch_wall_segment_trigger
(
  SectorID sector, WallID wall, u8 segment
)
{
  const SectorData& sd = map.sectors[sector];
  nc_assert(wall >= sd.int_data.first_wall && wall < sd.int_data.last_wall);
  WallRelID wrelid = cast<WallRelID>(wall - sd.int_data.first_wall);

  u32 idx = sector_wall_segment_to_u32(sector, wrelid, segment);
  auto it = segment_trigger_runtime.find(idx);
  if (it == segment_trigger_runtime.end())
  {
    return;
  }

  RuntimeSegmentInfo& sinfo = it->second;
  const TriggerData& td = triggers[sinfo.trigger];

  bool changed = true;

  if (!sinfo.trigger)
  {
    // Turn on and set cooldown potentially
    sinfo.triggered = true;

    if (td.has_timeout())
    {
      sinfo.countdown = td.timeout;
    }

    changed = true;
  }
  else if (td.can_turn_off)
  {
    // Turn off
    sinfo.triggered = false;
    changed = true;
  }

  if (changed && sector_change_callback)
  {
    sector_change_callback(sector);
  }
}

//==============================================================================
void MapDynamics::evaluate_activators
(
  std::vector<u16>& activator_values, f32 delta
)
{
  nc_assert(triggers.size() <= MAX_TRIGGERS);
  nc_assert(activator_values.size() == activators.size());

  TriggerID trigger_cnt = cast<TriggerID>(triggers.size());

  // Now iterate all sectors that can be activated and move them correctly
  for (TriggerID trigger_id = 0; trigger_id < trigger_cnt; ++trigger_id)
  {
    TriggerData& td = triggers[trigger_id];
    ActivatorID activator = td.activator;

    switch (td.type)
    {
      // Handle sectors
      case TriggerData::sector:
      {
        SectorID sid = td.sector_type.sector;
        mapping.for_each_in_sector(sid, [&](EntityID id, mat4)
        {
          bool player = id.type == EntityTypes::player && td.player_sensitive;
          bool enemy  = id.type == EntityTypes::enemy  && td.enemy_sensitive;
          activator_values[activator] += (player | enemy);
        });
      }
      break;

      // Handle walls
      case TriggerData::wall:
      {
        u32 idx = sector_wall_segment_to_u32
        (
          td.wall_type.sector,
          td.wall_type.wall,
          td.wall_type.segment
        );

        nc_assert(segment_trigger_runtime.contains(idx));
        RuntimeSegmentInfo& info = segment_trigger_runtime[idx];

        if (td.has_timeout())
        {
          lerp_towards(info.countdown, 0.0f, delta);
          if (info.countdown == 0.0f)
          {
            info.triggered = false;
          }
        }

        activator_values[activator] += info.triggered;
      }
      break;

      // Handle entities
      case TriggerData::entity:
      {
        bool exists = registry.get_entity(td.entity_type.entity);
        activator_values[activator] += (exists == td.while_alive);
      }
      break;
    }
  }
}

//==============================================================================
void MapDynamics::update(f32 delta)
{
  // Now iterate all sectors that can be activated and move them correctly
  nc_assert(activator_list.size() <= MAX_ACTIVATORS);
  ActivatorID activator_cnt = cast<ActivatorID>(activator_list.size());
  std::vector<u16> activator_values(activator_cnt, 0);

  // Iterate triggers
  evaluate_activators(activator_values, delta);

  // Iterate activators
  for (ActivatorID activator_id = 0; activator_id < activator_cnt; ++activator_id)
  {
    u16 threshold = activators[activator_id].threshold;
    u16 value     = activator_values[activator_id];

    bool is_on = value >= threshold;

    for (SectorID sid : activator_list[activator_id])
    {
      SectorData& sd = map.sectors[sid];

      f32 desired_floor    = sd.state_floors[is_on];
      f32 desired_ceil     = sd.state_ceils[is_on];
      f32 change_per_frame = delta * sd.move_speed;

      bool changed = false;

      if (sd.floor_height != desired_floor && change_per_frame != 0)
      {
        lerp_towards(sd.floor_height, desired_floor, change_per_frame);
        changed = true;
      }

      if (sd.ceil_height != desired_ceil && change_per_frame != 0)
      {
        lerp_towards(sd.ceil_height, desired_ceil, change_per_frame);
        changed = true;
      }

      if (changed && sector_change_callback)
      {
        sector_change_callback(sid);
      }
    }
  }
}

}
