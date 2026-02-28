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

// TODO:
// - topo sort for triggers based on their activator dependency

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
    if (td.type != TriggerData::wall)
    {
      continue;
    }

    u32 idx = sector_wall_segment_to_u32
    (
      td.wall_type.sector, td.wall_type.wall, td.wall_type.segment
    );

    nc_assert(!segment_trigger_runtime.contains(idx));
    segment_trigger_runtime.insert({idx, RuntimeSegmentInfo{.trigger = tid}});
  }

  // Map sectors to their activators
  nc_assert(map.sectors.size() < MAX_SECTORS);
  nc_assert(std::ranges::all_of(activators, [](const ActivatorData& activator)->bool { return activator.affected_sectors.empty(); }));

  SectorID sector_cnt = cast<SectorID>(map.sectors.size());
  for (SectorID sid = 0; sid < sector_cnt; ++sid)
  {
    const SectorData& sd = map.sectors[sid];

    if (sd.activator != INVALID_ACTIVATOR_ID)
    {
      nc_assert(sd.activator < activators.size());
      activators[sd.activator].affected_sectors.push_back(sid);
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
  nc_assert(sinfo.trigger != INVALID_TRIGGER_ID);
  const TriggerData& td = triggers[sinfo.trigger];

  if (!sinfo.triggered)
  {
    // Turn on and set cooldown potentially
    sinfo.triggered = true;
    sinfo.dirty = true;

    if (td.has_timeout())
    {
      sinfo.countdown = td.timeout;
    }
  }
  else if (td.can_turn_off)
  {
    // Turn off
    sinfo.triggered = false;
    sinfo.dirty = true;
  }
}

//==============================================================================
void MapDynamics::evaluate_activators
(
  std::vector<u16>& activator_values, f32 delta, bool notify
)
{
  nc_assert(triggers.size() <= MAX_TRIGGERS);
  nc_assert(activator_values.size() == activators.size());

  TriggerID trigger_cnt = cast<TriggerID>(triggers.size());

  // Now iterate all triggers
  for (TriggerID trigger_id = 0; trigger_id < trigger_cnt; ++trigger_id)
  {
    const TriggerData& td = triggers[trigger_id];
    u16& activator_value = activator_values[td.activator];

    switch (td.type)
    {
      // Handle sectors
      case TriggerData::sector:
      {
        SectorID sid = td.sector_type.sector;
        // Increment the activator for each relevant entity that is inside this sector
        mapping.for_each_in_sector(sid, [&](EntityID id, mat4)
        {
          const bool player = td.player_sensitive && (id.type == EntityTypes::player);
          const bool enemy  = td.enemy_sensitive && (id.type == EntityTypes::enemy);
          activator_value += (player || enemy);
        });
      }
      break;

      // Handle walls
      case TriggerData::wall:
      {
        const u32 idx = sector_wall_segment_to_u32
        (
          td.wall_type.sector,
          td.wall_type.wall,
          td.wall_type.segment
        );

        nc_assert(segment_trigger_runtime.contains(idx));
        RuntimeSegmentInfo& info = segment_trigger_runtime[idx];

        if (td.has_timeout() && info.triggered)
        {
          lerp_towards(info.countdown, 0.0f, delta);
          if (info.countdown == 0.0f)
          {
            info.triggered = false;
            info.dirty     = true;
          }
        }

        if (notify && info.dirty)
        {
          WallRelID wrelid = td.wall_type.wall;

          const SectorData& sd = map.sectors[td.wall_type.sector];
          WallData& wd = map.walls[wrelid + sd.int_data.first_wall];

          SurfaceData& ssd = wd.surface.surfaces[td.wall_type.segment].surface;
          TextureID tids[2] = {ssd.texture_id_default, ssd.texture_id_triggered};

          ssd.texture_id = tids[info.triggered];

          if (sector_change_callback)
          {
            sector_change_callback(td.wall_type.sector);
          }

          info.dirty = false;
        }

        activator_value += info.triggered;
      }
      break;

      // Handle entities
      case TriggerData::entity:
      {
        bool exists = registry.get_entity(td.entity_type.entity);
        activator_value += (exists == td.while_alive);
      }
      break;
    }
  }
}

//==============================================================================
void MapDynamics::update(f32 delta)
{
  // Now iterate all sectors that can be activated and move them correctly
  nc_assert(activators.size() <= MAX_ACTIVATORS);
  ActivatorID activator_cnt = cast<ActivatorID>(activators.size());
  std::vector<u16> activator_values(activator_cnt, 0);

  // Iterate triggers
  evaluate_activators(activator_values, delta, true);

  // Iterate activators
  for (ActivatorID activator_id = 0; activator_id < activator_cnt; ++activator_id)
  {
    ActivatorData& activator = activators[activator_id];
    u16 threshold = activator.threshold;
    u16 value     = activator_values[activator_id];

    bool is_on = value >= threshold;

    // Update affected sectors
    for (SectorID sid : activators[activator_id].affected_sectors)
    {
      SectorData& sector = map.sectors[sid];

      const f32 desired_floor    = sector.state_floors[is_on];
      const f32 desired_ceil     = sector.state_ceils[is_on];
      const f32 change_per_frame = delta * sector.move_speed;

      bool changed = false;

      if (sector.floor_height != desired_floor && change_per_frame != 0)
      {
        lerp_towards(sector.floor_height, desired_floor, change_per_frame);
        changed = true;
      }

      if (sector.ceil_height != desired_ceil && change_per_frame != 0)
      {
        lerp_towards(sector.ceil_height, desired_ceil, change_per_frame);
        changed = true;
      }

      if (changed && sector_change_callback)
      {
        // The sector changed.. Notify potential listener.
        sector_change_callback(sid);

        // Mark the surrounding sectors as dirty as well
        map.for_each_portal_of_sector(sid, [&](WallID wid)
        {
          nc_assert(map.walls[wid].portal_sector_id != INVALID_SECTOR_ID);
          sector_change_callback(map.walls[wid].portal_sector_id);
        });
      }
    }

    // Update hooks
    const bool activeness_did_change = (is_on != activator.is_active);
    activator.is_active = is_on;
    if (is_on || activeness_did_change) {
      for (const std::unique_ptr<IActivatorHook>& hook : activator.hooks) {
        if (activeness_did_change && is_on)
          hook->on_activated_start();
        if(is_on)
          hook->on_activated_update(delta);
        if (activeness_did_change && (!is_on))
          hook->on_activated_end();
      }
    }



  }
}

}
