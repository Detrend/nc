// Project Nuclidean Source File
#include <game/projectile.h>

#include <engine/game/game_system.h>
#include <engine/game/game_helpers.h>

#include <engine/entity/entity_type_definitions.h>
#include <engine/map/physics.h>
#include <engine/entity/entity_system.h>

#include <engine/player/player.h>
#include <engine/enemies/enemy.h>

#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>

#include <game/projectiles.h>
#include <game/particle.h>

#include <engine/sound/sound_system.h>
#include <engine/sound/sound_resources.h>

#include <math/lingebra.h>

#include <format>
#include <algorithm> // std::fill

namespace nc
{

//==============================================================================
/*static*/ EntityType Projectile::get_type_static()
{
  return EntityTypes::projectile;
}

//==============================================================================
/*static*/ EntityStatFlags Projectile::get_static_flags()
{
  return EntityStaticFlags::save_load;
}

//==============================================================================
void Projectile::init
(
  vec3 pos, vec3 dir, EntityID author, ProjectileType type
)
{
  const ProjectileStats& stats = PROJECTILE_STATS[type];

  std::fill(std::begin(m_penetrated_entities), std::end(m_penetrated_entities), INVALID_ENTITY_ID);

  Entity::init(pos, stats.radius, stats.radius * 2.0f);
  this->m_author   = author;
  this->m_velocity = normalize(dir) * stats.speed;
  this->m_type     = type;

  m_hit_cnt_remaining = stats.bounce_cnt + 1;

  m_appear = Appearance
  {
    .sprite   = "", // Will be immediately set by the update_appearance below
    .scale    = 30.0f,
    .mode     = Appearance::SpriteMode::mono,
    .rotation = Appearance::RotationMode::full,
  };

  // Set the proper sprite
  this->update_appearance();
}

//==============================================================================
void Projectile::update(f32 dt)
{
  auto& game = GameSystem::get();
  auto  lvl  = game.get_level();

  m_lifetime += dt;
  this->update_appearance();

  vec3 position  = this->get_position();
  mat4 transform = identity<mat4>();

  f32 r = this->get_radius();
  f32 h = this->get_height();

  // TODO: Player can't hit himself right now
  bool player_authored = m_author.type == EntityTypes::player;
  EntityTypeMask COLLIDE_WITH = player_authored
    ? (EntityTypeFlags::enemy)
    : (EntityTypeFlags::enemy | EntityTypeFlags::player);

  bool was_entity_hit = false;
  bool was_wall_hit   = false;

  u64 max_pen_cnt  = PROJECTILE_STATS[m_type].penetration_cnt;
  f32 bounce_coeff = PROJECTILE_STATS[m_type].bounce_cnt > 0 ? 1.0f : 0.0f;

  lvl.move_particle
  (
    position, m_velocity, transform, dt, r, h, 0.0f,
    bounce_coeff, COLLIDE_WITH,
    [&](const CollisionHit& hit)
    {
      if (hit.type == CollisionHit::entity && this->on_entity_hit(hit))
      {
        was_entity_hit = true;

        // check if penetration is possible
        for (u64 i = 0; i < min(MAX_PENETRATION_CNT, max_pen_cnt); ++i)
        {
          if (m_penetrated_entities[i] == INVALID_ENTITY_ID)
          {
            m_penetrated_entities[i] = hit.hit.entity.entity_id;
            return true; // continue
          }
        }

        m_hit_cnt_remaining = 0;
        return false;
      }
      else if (hit.type == CollisionHit::sector)
      {
        m_hit_cnt_remaining = min(m_hit_cnt_remaining - 1, m_hit_cnt_remaining);
        if (m_hit_cnt_remaining)
        {
          // play the ricochet snd
          GameHelpers::get().play_3d_sound
          (
            position, Sounds::ricochet, 32.0f, 0.3f
          );

          return true;
        }
        else
        {
          was_wall_hit = true;
          return false;
        }
      }

      return true;
    }
  );

  this->set_position(position);

  if (was_entity_hit)
  {
    game.get_entities().create_entity<Particle>
    (
      this->get_position(), "blood_splatter1",
      5, 0.3f, colors::BLACK, 0.0f, 24.0f
    );
  }

  if (m_hit_cnt_remaining == 0 && was_wall_hit && PROJECTILE_STATS[m_type].hit_sprite)
  {
    // Spawn destroy particle if necessary
    game.get_entities().create_entity<Particle>
    (
      this->get_position(),
      PROJECTILE_STATS[m_type].hit_sprite,
      PROJECTILE_STATS[m_type].hit_sprite_cnt,
      PROJECTILE_STATS[m_type].hit_sprite_len,
      PROJECTILE_STATS[m_type].hit_sprite_col,
      PROJECTILE_STATS[m_type].hit_sprite_rad
    );
  }

  if (m_hit_cnt_remaining == 0)
  {
    // Kill ourselves
    game.get_entities().destroy_entity(this->get_id());
  }
}

//==============================================================================
bool Projectile::on_entity_hit(const CollisionHit& hit)
{
  auto& game = GameSystem::get();
  nc_assert(hit.type == CollisionHit::entity);

  EntityID id = hit.hit.entity.entity_id;
  for (u64 i = 0; i < MAX_PENETRATION_CNT; ++i)
    if (m_penetrated_entities[i] == id) return false;

  if (Entity* entity = game.get_entities().get_entity(id))
  {
    s32 dmg = PROJECTILE_STATS[m_type].damage;

    switch (entity->get_type())
    {
      case EntityTypes::player:
      {
        entity->as<Player>()->damage(dmg);
        return true;
      }

      case EntityTypes::enemy:
      {
        entity->as<Enemy>()->damage(dmg, m_author);
        return true;
      }
    }
  }

  return false;
}

//==============================================================================
void Projectile::update_appearance()
{
  const auto& stats = PROJECTILE_STATS[this->m_type];

  if (stats.sprite_cnt > 1)
  {
    nc_assert(stats.sprite);
    f32 anim_prog  = stats.anim_len ? fmod(m_lifetime, stats.anim_len) : 0.0f;
    u32 sprite_idx = cast<u32>(stats.sprite_cnt * anim_prog);
    m_appear.sprite = std::format("{}_{}", stats.sprite, sprite_idx);
  }
  else
  {
    // TODO: is this necessary? How do we avoid this?
    m_appear.sprite = stats.sprite ? stats.sprite : "";
  }
}

//==============================================================================
const Appearance* Projectile::get_appearance() const
{
  return const_cast<Projectile*>(this)->get_appearance();
}

//==============================================================================
Appearance* Projectile::get_appearance()
{
  return m_appear.sprite ? &m_appear : nullptr;
}

//==============================================================================
EntityID Projectile::get_author_id() const
{
  return m_author;
}

}
