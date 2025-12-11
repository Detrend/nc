// Project Nucledian Source File
#include <game/projectile.h>

#include <engine/game/game_system.h>

#include <engine/entity/entity_type_definitions.h>
#include <engine/map/physics.h>
#include <engine/entity/entity_system.h>

#include <engine/player/player.h>
#include <engine/enemies/enemy.h>

#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>

#include <game/projectiles.h>

#include <engine/sound/sound_system.h>
#include <engine/sound/sound_resources.h>

#include <math/lingebra.h>

#include <format>

namespace nc
{

//==============================================================================
/*static*/ EntityType Projectile::get_type_static()
{
  return EntityTypes::projectile;
}

//==============================================================================
Projectile::Projectile
(
  vec3 pos, vec3 dir, EntityID author, ProjectileType type
)
: Entity    (pos, PROJECTILE_STATS[type].radius, PROJECTILE_STATS[type].radius * 2.0f)
, m_author  (author)
, m_velocity(normalize(dir) * PROJECTILE_STATS[type].speed)
, m_type    (type)
{
  using SprMode = Appearance::SpriteMode;
  const ProjectileStats& stats = PROJECTILE_STATS[type];

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

  constexpr EntityTypeMask DO_NOT_COLLIDE_WITH
    = EntityTypeFlags::projectile
    | EntityTypeFlags::point_light
    | EntityTypeFlags::pickup
    | EntityTypeFlags::prop;

  constexpr EntityTypeMask HIT_TYPES
    = PhysLevel::COLLIDE_ALL & ~DO_NOT_COLLIDE_WITH;

  lvl.move_particle
  (
    position, m_velocity, transform, dt, r, h, 0.0f,
    1.0f, HIT_TYPES,
    [&](const CollisionHit& hit)
    {
      if (hit.type == CollisionHit::entity && this->on_entity_hit(hit))
      {
        m_hit_cnt_remaining = 0;
        // play damage hit
      }
      else
      {
        m_hit_cnt_remaining = min(m_hit_cnt_remaining - 1, m_hit_cnt_remaining);
        if (m_hit_cnt_remaining)
        {
          // play the ricochet snd
          SoundSystem::get().play(Sounds::ricochet, 0.3f);
        }
        else
        {
          // play destroy sound
        }
      }
    }
  );

  this->set_position(position);

  if (!m_hit_cnt_remaining)
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

  if (Entity* entity = game.get_entities().get_entity(hit.hit.entity.entity_id))
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
    f32 anim_prog  = stats.anim_len ? fmod(m_lifetime, stats.anim_len) : 0.0f;
    u32 sprite_idx = cast<u32>(stats.sprite_cnt * anim_prog);
    m_appear.sprite = std::format("{}_{}", stats.sprite, sprite_idx);
  }
  else
  {
    // TODO: is this necessary? How do we avoid this?
    m_appear.sprite = stats.sprite;
  }
}

//==============================================================================
const Appearance& Projectile::get_appearance() const
{
  return m_appear;
}

//==============================================================================
Appearance& Projectile::get_appearance()
{
  return m_appear;
}

//==============================================================================
EntityID Projectile::get_author_id() const
{
  return m_author;
}

}
