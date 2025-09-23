// Project Nucledian Source File
#include <game/projectile.h>

#include <engine/player/thing_system.h>

#include <engine/entity/entity_type_definitions.h>
#include <engine/map/physics.h>
#include <engine/entity/entity_system.h>

#include <engine/player/player.h>
#include <engine/enemies/enemy.h>

#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>

#include <game/weapons.h>

#include <engine/sound/sound_system.h>
#include <engine/sound/sound_resources.h>

#include <math/lingebra.h>

namespace nc
{

//==============================================================================
EntityType Projectile::get_type_static()
{
  return EntityTypes::projectile;
}

//==============================================================================
Projectile::Projectile
(
  vec3 pos, vec3 dir, EntityID author, WeaponType type
)
: Entity            (pos, PROJECTILE_STATS[type].radius, PROJECTILE_STATS[type].radius * 2.0f)
, m_author          (author)
, m_velocity        (normalize(dir) * PROJECTILE_STATS[type].speed)
, m_type            (type)
{
  m_hit_cnt_remaining = PROJECTILE_STATS[type].bounce_cnt + 1;

  m_appear = Appearance
  {
    .sprite   = "plasma_ball",
    .scale    = 30.0f,
    .mode     = Appearance::SpriteMode::mono,
    .rotation = Appearance::RotationMode::full,
  };
}

//==============================================================================
void Projectile::update(f32 dt)
{
  auto& game = ThingSystem::get();
  auto  lvl  = game.get_level();

  vec3 position  = this->get_position();
  mat4 transform = identity<mat4>();

  f32 r = this->get_radius();
  f32 h = this->get_height();

  EntityTypeMask hit_types = PhysLevel::COLLIDE_ALL & ~(EntityTypeFlags::projectile);

  lvl.move_particle
  (
    position, m_velocity, transform, dt, r, h, 0.0f,
    1.0f, hit_types,
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
  auto& game = ThingSystem::get();
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
Transform Projectile::calc_transform() const
{
  return Transform{this->get_position()};
}

//==============================================================================
EntityID Projectile::get_author_id() const
{
  return m_author;
}

}
