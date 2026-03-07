// Project Nucledian Source File
#pragma once

#include <engine/entity/entity.h>
#include <engine/sound/sound_types.h>
#include <engine/sound/sound_handle.h>
#include <engine/map/map_types.h>

namespace nc
{

// TODO: The current implementation assumes that the sound never moves.
// TODO: We assume that the camera is always attached to the player. Will not
//       work otherwise.
class SoundEmitter : public Entity
{
public:
  static EntityType get_type_static();

  SoundEmitter
  (
    vec3    position,
    SoundID sound,
    f32     range,
    f32     volume = 1.0f,
    bool    loop   = false
  );

  // Updates the volume of the sound, kills it if necessary
  void update(f32 delta);

  void on_player_traversed_nc_portal(EntityID player, SectorID sid, WallID wid);

private:
  f32 calc_dist_to_listener() const;

  bool get_listener_pos(vec3& pos_out) const;

  static constexpr u64 NUM_PORTALS_TRACKED = 8;

  // Identifies one portal
  struct PortalIdentity
  {
    SectorID sector = INVALID_SECTOR_ID;
    WallID   wall   = INVALID_WALL_ID;
  };

  PortalIdentity m_portals_to_player[NUM_PORTALS_TRACKED]{};
  SoundHandle    m_handle; // Invalidates on save/load
  u8             m_num_portals = 0;
  f32            m_range       = 0.0f;
  f32            m_volume      = 1.0f;
};

}
