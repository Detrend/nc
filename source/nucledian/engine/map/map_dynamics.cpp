// Project Nucledian Source File

#include <engine/map/map_dynamics.h>
#include <engine/map/map_system.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_types.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>

#include <engine/sound/sound_emitter.h>
#include <engine/sound/sound_resources.h>

#include <math/lingebra.h>
#include <math/utils.h>

#include <algorithm> // std::count_if

// TODO:
// - topo sort for triggers based on their activator dependency

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
void MapDynamics::on_destroy()
{
  // Kill all sounds
  for (SectorID sid = 0; sid < cast<SectorID>(sector_sounds.size()); ++sid)
  {
    this->on_sector_moving_changed(sid, false);
  }
}

//==============================================================================
void MapDynamics::on_map_rebuild_and_entities_created()
{
  nc_assert(triggers.size() <= MAX_TRIGGERS);

  // Map sectors to their activators
  nc_assert(map.sectors.size() < MAX_SECTORS);
  nc_assert(std::ranges::all_of(activators, [](const ActivatorData& activator)->bool { return activator.affected_sectors.empty(); }));

  moving_sectors.resize(map.sectors.size(), false);
  sector_sounds.resize(map.sectors.size(), INVALID_ENTITY_ID);

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
SegmentID MapDynamics::segment_id_from_trigger(const TriggerData& td) const
{
  nc_assert(td.type == TriggerData::wall);

  const SectorData& sd = map.sectors[td.wall_type.sector];
  const WallData&   wd = map.walls[sd.first_wall + td.wall_type.wall];

  return wd.first_segment + td.wall_type.segment;
}

//==============================================================================
void MapDynamics::on_sector_moving_changed(SectorID sid, bool started_moving)
{
  if (started_moving)
  {
    nc_assert(sector_sounds[sid] == INVALID_ENTITY_ID);

    vec2 avg_pos_2d = VEC2_ZERO;
    map.for_each_wall_of_sector(sid, [&](WallID wall)
    {
      avg_pos_2d += map.walls[wall].pos;
    });
    avg_pos_2d /= map.sectors[sid].last_wall - map.sectors[sid].first_wall;

    f32 floor = map.sectors_dynamic[sid].floor_height;
    f32 roof  = map.sectors_dynamic[sid].ceil_height;
    f32 avg_h = (floor + roof) * 0.5f;

    vec3 sound_pos = vec3{avg_pos_2d.x, avg_h, avg_pos_2d.y};

    SoundEmitter* emitter = registry.create_entity<SoundEmitter>
    (
      sound_pos, Sounds::door, 20.0f, 0.2f, true
    );

    nc_assert(emitter);

    sector_sounds[sid] = emitter->get_id();
  }
  else
  {
    // Kill the entity if it exists
    EntityID id = sector_sounds[sid];
    if (id != INVALID_ENTITY_ID)
    {
      if (SoundEmitter* emitter = registry.get_entity<SoundEmitter>(id))
      {
        emitter->kill();
      }
    }

    sector_sounds[sid] = INVALID_ENTITY_ID;
  }
}

//==============================================================================
bool MapDynamics::switch_wall_segment_trigger
(
  SectorID sector, WallID wall, u8 segment, bool& turned_on
)
{
  const SectorData& sd = map.sectors[sector];
  nc_assert(wall >= sd.first_wall && wall < sd.last_wall);
  WallRelID wrelid = cast<WallRelID>(wall - sd.first_wall);

  auto it = std::ranges::find_if(triggers, [&](const TriggerData& td)
  {
    return td.type == TriggerData::wall
      && td.wall_type.sector == sector
      && td.wall_type.wall == wrelid
      && td.wall_type.segment == segment;
  });

  if (it == triggers.end())
  {
    return false;
  }

  const TriggerData& td = *it;
  SegmentID seg_id = segment_id_from_trigger(td);

  WallSegmentDynData& sinfo = map.wall_segments_dynamic[seg_id];

  if (!sinfo.triggered)
  {
    // Turn on and set cooldown potentially
    sinfo.triggered = true;
    sinfo.dirty     = true;
    turned_on       = true;

    if (td.has_timeout())
    {
      sinfo.countdown = td.timeout;
    }

    return true;
  }
  else if (td.can_turn_off)
  {
    // Turn off
    sinfo.triggered = false;
    sinfo.dirty     = true;
    turned_on       = false;
    return true;
  }

  return false;
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
        SegmentID seg_id = segment_id_from_trigger(td);
        WallSegmentDynData& info = map.wall_segments_dynamic[seg_id];

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
          [[maybe_unused]]const SectorData&  sd      = map.sectors[td.wall_type.sector];
          [[maybe_unused]]WallRelID          wrelid  = td.wall_type.wall;
          [[maybe_unused]]WallID             wid     = sd.first_wall + wrelid;
          [[maybe_unused]]const SurfaceData& surface = map.wall_segments[seg_id].surface;

          if (surface.texture_id_default != surface.texture_id_triggered)
          {
            nc_warn(
              "Wall segment with no alt texture triggered."
              "Sector: {}, wall rel: {}, wall abs: {}, seg rel: {}",
              td.wall_type.sector, wrelid, wid, td.wall_type.segment
            );
          }

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
      SectorData&    sector     = map.sectors[sid];
      SectorDynData& sector_dyn = map.sectors_dynamic[sid];

      const f32 desired_floor    = sector.state_floors[is_on];
      const f32 desired_ceil     = sector.state_ceils[is_on];
      const f32 change_per_frame = delta * sector.move_speed;

      bool moved = false;

      if (sector_dyn.floor_height != desired_floor && change_per_frame != 0)
      {
        lerp_towards(sector_dyn.floor_height, desired_floor, change_per_frame);
        moved = true;
      }

      if (sector_dyn.ceil_height != desired_ceil && change_per_frame != 0)
      {
        lerp_towards(sector_dyn.ceil_height, desired_ceil, change_per_frame);
        moved = true;
      }

      if (moved && sector_change_callback)
      {
        // The sector changed.. Notify potential listener.
        sector_change_callback(sid);
      }

      if (moving_sectors[sid] != moved)
      {
        // Notify that it started/stopped moving
        on_sector_moving_changed(sid, moved);
        moving_sectors[sid] = moved;
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
