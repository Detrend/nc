// Project Nucledian Source File
#pragma once

#include <engine/sound/sound_system.h>
#include <engine/sound/sound_emitter.h>
#include <engine/player/player.h>

#include <engine/game/game_system.h>
#include <engine/game/game_helpers.h>

#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>

#include <engine/map/map_system.h>
#include <engine/map/physics.h>

#include <engine/graphics/debug/gizmo.h>

#include <intersect.h>

namespace nc
{

//==============================================================================
/*static*/ EntityType SoundEmitter::get_type_static()
{
  return EntityTypes::sound_emitter;
}

//==============================================================================
SoundEmitter::SoundEmitter
(
  vec3    position,
  SoundID sound,
  f32     range,
  f32     volume,
  bool    loop 
)
: Entity(position, 0.0f)
, m_range(range)
, m_volume(volume)
{
  // Calculate the portals between us and the player
  vec3 listener_pos = VEC3_ZERO;
  if (!this->get_listener_pos(listener_pos))
  {
    return;
  }

  const MapSectors& map = GameSystem::get().get_map();
  PhysLevel         lvl = GameSystem::get().get_level();

  // Find some sector nearby the spot we spawned in
  vec3 from_pos = this->get_position();
  if (!map.get_sector_from_point(from_pos.xz()))
  {
    // Do a more broad query around.. Find some match nearby
    vec2  offset = vec2{1.0f, 1.0f};
    aabb2 bbox   = aabb2{from_pos.xz() + offset, from_pos.xz() - offset};

    SectorID best_sid  = INVALID_SECTOR_ID;
    vec2     best_pt   = VEC2_ZERO;
    f32      best_dist = FLT_MAX;

    map.sector_grid.query_aabb(bbox, [&](aabb2, SectorID sid)
    {
      vec2 closest = map.closest_point_of_sector_2d(from_pos.xz(), sid);
      f32  dist    = distance(closest, from_pos.xz());

      if (dist < best_dist)
      {
        best_dist = dist;
        best_sid  = sid;
        best_pt   = closest;
      }

      return true;
    });

    if (best_sid == INVALID_SECTOR_ID)
    {
      // Did not find any close sector nearby
      nc_warn(
        "Spawned a sound out of bounds. Pos: [{},{},{}]",
        position.x, position.y, position.z);
      return;
    }

    from_pos = vec3{best_pt.x, from_pos.y, best_pt.y};
  }

  // Calculate the path
  std::vector<PhysLevel::PortalSector> nc_portals;
  if (!lvl.calc_path_and_list_nc_portals(from_pos, listener_pos, nc_portals, m_range))
  {
    nc_warn(
      "Spawned a sound with not path to the player. Pos: [{},{},{}]",
      position.x, position.y, position.z);
    return;
  }

  // Transform into our own storage
  m_num_portals = cast<u8>(min(nc_portals.size(), NUM_PORTALS_TRACKED));
  for (u64 idx = 0; idx < m_num_portals; ++idx)
  {
    m_portals_to_player[idx].sector = nc_portals[idx].sector_id;
    m_portals_to_player[idx].wall   = nc_portals[idx].wall_id;
  }

  // Calculate the volume now that we stored the portals
  f32 total_dist = this->calc_dist_to_listener();
  f32 new_vol    = (1.0f - clamp(total_dist / m_range, 0.0f, 1.0f)) * m_volume;

  // Spawn the sound proxy
  m_handle = SoundSystem::get().play(sound, new_vol, loop);
}

//==============================================================================
bool SoundEmitter::get_listener_pos(vec3& pos_out) const
{
  Player* player = GameHelpers::get().get_player();
  if (!player)
  {
    return false;
  }

  pos_out = player->get_eye_pos();
  return true;
}

//==============================================================================
void SoundEmitter::on_player_traversed_nc_portal
(
  EntityID, SectorID sid, WallID wid
)
{
  const MapSectors& map = GameSystem::get().get_map();
  WallID   op_wid = map_helpers::get_nc_opposing_wall(map, sid, wid);
  SectorID op_sid = map.walls[wid].portal_sector_id;

  for (u8 idx = 0; idx < m_num_portals; ++idx)
  {
    SectorID sp = m_portals_to_player[idx].sector;
    WallID   wp = m_portals_to_player[idx].wall;

    bool same_as_incoming = sp == sid    && wp == wid;
    bool same_as_opposite = sp == op_sid && wp == op_wid;

    if (same_as_incoming || same_as_opposite)
    {
      // We returned to a portal we traversed previously, remove all further
      // ones from the stack
      m_num_portals = idx;
      return;
    }
  }

  if (m_num_portals < NUM_PORTALS_TRACKED)
  {
    m_portals_to_player[m_num_portals].sector = sid;
    m_portals_to_player[m_num_portals].wall   = wid;
    m_num_portals++;
  }
}

//==============================================================================
f32 SoundEmitter::calc_dist_to_listener() const
{
  const MapSectors& map = GameSystem::get().get_map();

  vec3 start_pos    = this->get_position();
  vec3 listener_pos = VEC3_ZERO;
  if (!this->get_listener_pos(listener_pos))
  {
    // Did not find the listener
    return FLT_MAX;
  }

  f32 total_dist = 0.0f;

  // Calculate the distance from us to the player through all portals between us
  vec3 prev_pos = start_pos;

  nc_assert(m_num_portals <= NUM_PORTALS_TRACKED);

  for (u64 idx = 0; idx < m_num_portals; ++idx)
  {
    const PortalIdentity& portal = m_portals_to_player[idx];
    SectorID sector = portal.sector;
    WallID   wall   = portal.wall;

    vec2 p1 = map.walls[wall].pos;
    vec2 p2 = map.walls[map_helpers::next_wall(map, sector, wall)].pos;

    // Calculate the portal frame coords
    f32  portal_offset = map.walls[wall].get_ground_offset();
    f32  floor_h = map.sectors_dynamic[sector].floor_height + portal_offset;
    f32  ceil_h  = map.sectors_dynamic[sector].floor_height;

    vec2 closest_2d = dist::closest_point_on_the_line(prev_pos.xz(), p1, p2);
    f32  closest_h  = clamp(prev_pos.y, floor_h, ceil_h);

    vec3 this_pos = vec3{closest_2d.x, closest_h, closest_2d.y};

    // Increase the distance
    total_dist += distance(prev_pos, this_pos);

#ifdef NC_DEBUG_DRAW
    // Debug
    Gizmo::create_line_2d("Sounds", this_pos.xz(), prev_pos.xz(), colors::PINK);
#endif

    // Transform by the portal
    mat4 portal_trans = map.calc_portal_to_portal_projection(sector, wall);
    prev_pos = (portal_trans * vec4{this_pos, 1.0f}).xyz();
  }

#ifdef NC_DEBUG_DRAW
  // Debug
  Gizmo::create_line_2d("Sounds", listener_pos.xz(), prev_pos.xz(), colors::PINK);
#endif

  total_dist += distance(prev_pos, listener_pos);

  return total_dist;
}

//==============================================================================
void SoundEmitter::update([[maybe_unused]]f32 delta)
{
  if (!m_handle.is_valid())
  {
    // Destroy ourselves
    this->kill();
    return;
  }

  f32 total_dist = this->calc_dist_to_listener();

  // Now that we have the total distance we can calculate the volume
  f32 new_vol = (1.0f - clamp(total_dist / m_range, 0.0f, 1.0f)) * m_volume;

  // And set it
  m_handle.set_volume(new_vol);
}

//==============================================================================
void SoundEmitter::kill()
{
  if (m_handle.is_valid())
  {
    m_handle.kill();
  }

  GameSystem::get().get_entities().destroy_entity(this->get_id());
}

}
