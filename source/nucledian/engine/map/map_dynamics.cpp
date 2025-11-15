// Project Nucledian Source File

#include <engine/map/map_dynamics.h>
#include <engine/map/map_system.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>

#include <math/lingebra.h>
#include <math/utils.h>

#include <algorithm> // std::count_if

namespace nc
{

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
void MapDynamics::on_map_rebuild()
{
  
}

//==============================================================================
void MapDynamics::update(f32 delta)
{
  #define DO_SANITY_CHECK 1

  // Now iterate all sectors that can be activated and move them correctly
  for (auto& item : activator_to_sector_map)
  {
    ActivatorID activator_id = item.first;
    ActivatorLookData& look_data = item.second;

    // For each activator we have to evaluate if it is ON or OFF. We do this each
    // frame.
    u16 activator_threshold = map.activators[activator_id].threshold;
    nc_assert(activator_threshold > 0);
    u16 activator_value = 0;

    // Iterate all sectors that have triggers and check if they are triggered.
    for (SectorID sid : look_data.sector_watch_list)
    {
      const SectorData& sd = map.sectors[sid];

#if DO_SANITY_CHECK
      [[maybe_unused]]auto* first_ptr = &map.triggers[sd.first_trigger];
      [[maybe_unused]]auto* last_ptr  = &map.triggers[sd.last_trigger];
      [[maybe_unused]]u64 cnt = std::count_if
      (
        first_ptr, last_ptr, [&](const TriggerData& td)
        {
          return td.activator == activator_id;
        }
      );
      nc_assert(cnt == 1);
#endif

      for (TriggerID tid = sd.first_trigger; tid < sd.last_trigger; ++tid)
      {
        const TriggerData& td = map.triggers[tid];

#if DO_SANITY_CHECK
        nc_assert(td.player_sensitive || td.enemy_sensitive);
#endif

        if (td.activator == activator_id)
        {
          mapping.for_each_in_sector(sid, [&](EntityID id, mat4)
          {
            bool player = id.type == EntityTypes::player && td.player_sensitive;
            bool enemy  = id.type == EntityTypes::enemy && td.enemy_sensitive;
            if (player || enemy)
            {
              activator_value += 1;
              return;
            }
          });

          break;
        }
      }
    }

    // Iterate all wall segments and update their timings.
    using Sinfo = ActivatorLookData::SegmentInfo;
    for (Sinfo& segment : look_data.segment_watch_list)
    {
      // Update the countdown if required and potentially turn it off
      if (segment.triggered)
      {
        const WallSegmentData& sd = map.walls[segment.wall].surface;
        TriggerID tid = sd.surfaces[segment.segment_idx].trigger;
        const TriggerData& td = map.triggers[tid];

        if (td.has_timeout())
        {
          lerp_towards(segment.countdown, 0.0f, delta);
        }

        if (segment.countdown == 0.0f)
        {
          segment.triggered = false;
          callback(segment.sector);
        }
      }

      // Is it still triggered? Count it if so.
      if (segment.triggered)
      {
        activator_value += 1;
      }
    }

    // Iterate all the enemies/items and check if they exist.
    for (EntityID watch_entity : look_data.entity_watch_list)
    {
      if (!registry.get_entity(watch_entity))
      {
        activator_value += 1;
      }
    }

    bool is_on = activator_value >= activator_threshold;

    for (SectorID sid : look_data.influenced_sectors)
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

      if (changed && callback)
      {
        callback(sid);
      }
    }
  }
}

}
