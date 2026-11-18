// Project Nuclidean Source File
#include <common.h>

#include <engine/entity/entity_types.h>
#include <engine/game/game.h>

// Entity types
#include <engine/network/constants.h>
#include <engine/network/network_system.h>
#include <engine/player/player.h>
#include <engine/enemies/enemy.h>
#include <engine/sound/sound_emitter.h>
#include <game/projectile.h>
#include <game/particle.h>
#include <game/teleport.h>

// Other
#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>
#include <engine/entity/sector_mapping.h>
#include <engine/game/game_system.h>
#include <engine/map/map_dynamics.h>
#include <engine/map/map_system.h>
#include <game/entity_attachment_manager.h>

#include <profiling.h>

#include <engine/input/game_input.h>
#include <buffer.h>

namespace nc
{

// Time from full wipe to respawn at start in seconds.
constexpr f32 RESPAWN_DELAY = 2.0f;

//==============================================================================
void Game::on_destroy()
{
  if (dynamics)
  {
    dynamics->on_destroy();
  }
}

//==============================================================================
void Game::update
(
  f32               dt,
  const InputArray& curr_inputs,
  const InputArray& prev_inputs
)
{
  NC_SCOPE_COUNTER(game_update);

  // Init the players with transition data on the first frame. Same code path as
  // when playing a demo.
  if (frame_idx == 0)
  {
    for (PlayerID player_id = 0; player_id < MAX_PLAYER_COUNT; ++player_id)
    {
      Player* const player = entities->get_entity<Player>(player_ids[player_id]);
      if (player && !transition_data[player_id].is_empty())
      {
        player->init_with_level_transition_data(transition_data[player_id]);
      }
    }
  }

  time_since_start += dt;

  // Handle the players first
  {
    NC_SCOPE_COUNTER(player_update)
    for (PlayerID player_id = 0; player_id < MAX_PLAYER_COUNT; ++player_id)
    {
      if (player_ids[player_id] == INVALID_ENTITY_ID)
        continue;

      Player* const player = entities->get_entity<Player>(player_ids[player_id]);
      nc_assert(player, "player slot {} holds a destroyed entity", player_id);
      player->update(curr_inputs[player_id], prev_inputs[player_id], dt);
    }
  }

  if (NetworkSystem::get().is_multiplayer())
  {
    handle_revives(curr_inputs, dt);
    handle_respawn_after_wipe();
  }

  // Handle enemies
  {
    NC_SCOPE_COUNTER(enemy_update)
    entities->for_each<Enemy>([&](Enemy& enemy)
    {
      enemy.update(dt);
    });
  }

  // Handle projectiles
  {
    NC_SCOPE_COUNTER(projectile_update)
    entities->for_each<Projectile>([&](Projectile& proj)
    {
      proj.update(dt);
    });
  }

  // Handle teleports
  entities->for_each<Teleport>([&](Teleport& teleport)
  {
    teleport.update(dt);
  });

  // Handle particles
  entities->for_each<Particle>([&](Particle& particle)
  {
    particle.update(dt);
  });

  // Handle sound
  entities->for_each<SoundEmitter>([&](SoundEmitter& sound)
  {
    sound.update(dt);
  });

  // And update the map
  {
    NC_SCOPE_COUNTER(dynamics_update)
    dynamics->update(dt);
  }

  // Push the frame index
  frame_idx += 1;

  // And then clean up the dead entities
  entities->cleanup();
}

//==============================================================================
void Game::handle_revives(const InputArray& current_inputs, f32 delta_time)
{
  for (EntityID dead_id : player_ids)
  {
    Player* const dead = entities->get_entity<Player>(dead_id);
    if (!dead || dead->is_alive())
    {
      continue;
    }

    bool being_revived = false;
    for (PlayerID player_id = 0; player_id < MAX_PLAYER_COUNT; ++player_id)
    {
      const Player* const teammate = entities->get_entity<Player>(player_ids[player_id]);
      const bool          use_held = current_inputs[player_id].keys & 1 << PlayerKeyInputs::use;
      being_revived |= teammate && use_held && teammate->can_revive(*dead);

      if (being_revived)
      {
        break;
      }
    }

    dead->update_revive(being_revived, delta_time);
  }
}

//==============================================================================
void Game::handle_respawn_after_wipe()
{
  for (EntityID entity_id : player_ids)
  {
    const Player* const player = entities->get_entity<Player>(entity_id);
    if (player && (player->is_alive() || player->get_time_since_death() < RESPAWN_DELAY))
    {
      return;
    }
  }

  entities->for_each<Player>([&](Player& player)
  {
    player.respawn(spawn_point, spawn_direction);
  });
}

//==============================================================================
void Game::serialize(Buffer& buffer)
{
  // Small data first
  buffer.serialize_array(this->player_ids.data(), this->player_ids.size());
  buffer.serialize(this->frame_idx);
  buffer.serialize(this->time_since_start);
  // No need to serialize "is_level_completed" and "next_level_name"

  // Subsystems second
  map->serialize(buffer);
  entities->serialize(buffer);
  dynamics->serialize(buffer);
  attachment->serialize(buffer);

  // Rebuild the mapping manually, probably faster than loading it
  if (buffer.is_deserializing())
  {
    mapping->on_map_rebuild();
    entities->for_each(EntityTypeFlags::all, [&](Entity& entity)
    {
      mapping->on_entity_create
      (
        entity.get_id(),
        entity.get_position(),
        entity.get_radius(),
        entity.get_height()
      );
    });
  }
}

//==============================================================================
EntityID Game::get_local_player_id() const
{
  return player_ids[NetworkSystem::get().get_local_player_id()];
}

}
