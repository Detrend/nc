// Project Nuclidean Source File

#include <engine/game/game.h>

// Entity types
#include <engine/player/player.h>
#include <engine/enemies/enemy.h>
#include <engine/sound/sound_emitter.h>
#include <game/projectile.h>
#include <game/particle.h>
#include <game/teleport.h>

// Other
#include <engine/map/map_system.h>
#include <engine/map/map_dynamics.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_type_definitions.h>
#include <game/entity_attachment_manager.h>

#include <engine/input/game_input.h>
#include <buffer.h>

namespace nc
{

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
  f32                  dt,
  PlayerSpecificInputs curr_input,
  PlayerSpecificInputs prev_input
)
{
  // Init the player with transition data on the first frame. Same code path as
  // when playing a demo.
  if (frame_idx == 0 && !transition_data.is_empty())
  {
    if (Player* player = entities->get_entity<Player>(player_id))
    {
      player->init_with_level_transition_data(transition_data);
    }
  }

  time_since_start += dt;

  // Handle the player first
  entities->for_each<Player>([&](Player& player)
  {
    player.update(curr_input, prev_input, dt);
  });

  // Handle enemies
  entities->for_each<Enemy>([&](Enemy& enemy)
  {
    enemy.update(dt);
  });

  // Handle projectiles
  entities->for_each<Projectile>([&](Projectile& proj)
  {
    proj.update(dt);
  });

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
  dynamics->update(dt);

  // Push the frame index
  frame_idx += 1;

  // And then clean up the dead entities
  entities->cleanup();
}

//==============================================================================
void Game::serialize(Buffer& buffer)
{
  // Small data first
  buffer.serialize(this->player_id);
  buffer.serialize(this->frame_idx);
  buffer.serialize(this->time_since_start);
  buffer.serialize(this->difficulty);
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
    entities->for_each(EntityTypes::all, [&](Entity& entity)
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

}
