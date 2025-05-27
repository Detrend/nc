// Project Nucledian Source File
#include <game/projectile.h>

#include <engine/player/thing_system.h>

#include <engine/entity/entity_type_definitions.h>
#include <engine/map/physics.h>
#include <engine/entity/entity_system.h>

#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>

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
  auto& gfx = get_engine().get_module<GraphicsSystem>();

  m_appear = Appearance
  {
    .color     = colors::BLUE,
    .model     = gfx.get_cube_model(),
    .transform = Transform{VEC3_ZERO, vec3{size}}
  };
}

//==============================================================================
void Projectile::update(f32 dt)
{
  auto& game = ThingSystem::get();
  auto  lvl  = game.get_level();

  vec3 position = this->get_position();
  vec3 forward  = m_velocity;

  bool hit_something = false;

  lvl.move_and_collide
  (
    position, m_velocity, forward, dt, this->get_radius(), this->get_height(),
    0.0f, PhysLevel::COLLIDE_EVERYTHING, 0, 0.0f,
    [&hit_something](const RayHit& /*hit*/)
    {
      // stop the simulation on hit
      hit_something = true;
      return PhysLevel::CollisionReaction::stop_simulation;
    }
  );

  this->set_position(position);

  if (hit_something)
  {
    // kill ourselves
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

