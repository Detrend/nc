// Project Nuclidean Source File

#include <engine/game/game_helpers.h>
#include <engine/game/game.h>
#include <engine/game/game_system.h>

#include <engine/entity/entity_system.h>

#include <game/entity_attachment_manager.h>
#include <game/projectiles.h>

#include <engine/player/player.h>

#include <engine/sound/sound_emitter.h>
#include <engine/enemies/enemy.h>

#include <game/projectile.h>
#include <engine/graphics/entities/lights.h>

#include <engine/map/physics.h>

namespace nc
{

//==============================================================================
/*static*/ GameHelpers GameHelpers::get()
{
	return GameSystem::get().get_game_helpers();
}

//==============================================================================
GameHelpers::GameHelpers(Game& game)
: m_game(game)
{

}

//==============================================================================
u64 GameHelpers::get_frame_idx() const
{
  return m_game.frame_idx;
}

//==============================================================================
f64 GameHelpers::get_time_since_start() const
{
    return m_game.time_since_start;
}

//==============================================================================
Player* GameHelpers::get_player()
{
  return m_game.entities->get_entity<Player>(m_game.player_id);
}

//==============================================================================
PhysLevel GameHelpers::get_level() const
{
  return PhysLevel
  {
    .entities = *m_game.entities,
    .map      = *m_game.map,
    .mapping  = *m_game.mapping,
  };
}

//==============================================================================
Projectile* GameHelpers::spawn_projectile
(
  ProjectileType type, vec3 from, vec3 dir, EntityID author
)
{
  // Spawn projectile
  Projectile* projectile = m_game.entities->create_entity<Projectile>
  (
    from, dir, author, type 
  );

  if (vec3 light_col = PROJECTILE_STATS[type].light_color; light_col != VEC3_ZERO)
  {
    // And its light
    PointLight* light = m_game.entities->create_entity<PointLight>
    (
      from, 3.0f, 2.5f, 1.15f, light_col
    );

    // And attach it
    m_game.attachment->attach_entity
    (
      light->get_id(), projectile->get_id(), EntityAttachmentFlags::all
    );
  }

  return projectile;
}

//==============================================================================
void GameHelpers::play_3d_sound
(
  vec3 pos, SoundID sound, f32 distance, f32 volume
)
{
  m_game.entities->create_entity<SoundEmitter>(pos, sound, distance, volume);
}

//==============================================================================
void GameHelpers::on_player_traversed_nc_portal
(
  EntityID player, mat4 transform, SectorID sid, WallID wid
)
{
  m_game.entities->for_each<Enemy>([&](Enemy& enemy)
  {
    enemy.on_player_traversed_nc_portal(player, transform);
  });

  m_game.entities->for_each<SoundEmitter>([&](SoundEmitter& sound)
  {
    sound.on_player_traversed_nc_portal(player, sid, wid);
  });
}

}
