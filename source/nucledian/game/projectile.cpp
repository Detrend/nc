// Project Nucledian Source File
#include <game/projectile.h>

#include <engine/player/thing_system.h>

#include <engine/entity/entity_type_definitions.h>
#include <engine/map/physics.h>
#include <engine/entity/entity_system.h>

#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>

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
Projectile::Projectile(vec3 pos, vec3 dir, f32 size, bool player_projectile)
: Entity(pos, size, size)
, m_player_authored(player_projectile)
, m_velocity(dir)
{
  m_appear = Appearance
  {
    .texture = TextureManager::instance().get_test_plasma_texture(),
    .scale = 20.0f,
  };
}

//==============================================================================
void Projectile::update(f32 dt)
{
  auto& game = ThingSystem::get();
  auto  lvl  = game.get_level();

  vec3 position  = this->get_position();
  vec3 forward   = m_velocity;
  mat4 transform = identity<mat4>();

  f32 r = this->get_radius();
  f32 h = this->get_height();

  lvl.move_particle
  (
    position, m_velocity, transform, dt, r, h, 0.0f,
    1.0f, PhysLevel::COLLIDE_ALL,
    [&](const CollisionHit& /*hit*/)
    {
			m_hit_cnt_remaining = std::min(m_hit_cnt_remaining, m_hit_cnt_remaining - 1);
      if (m_hit_cnt_remaining)
      {
        // play the ricochet snd
        SoundSystem::get().play(Sounds::ricochet, 0.3f);
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

}
