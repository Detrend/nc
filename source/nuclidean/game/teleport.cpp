// Project Nuclidean Source File
#include <game/teleport.h>

#include <engine/entity/entity_type_definitions.h>
#include <engine/entity/entity_system.h>

#include <engine/game/game_system.h>

#include <game/particle.h>

namespace nc
{

constexpr vec3 OFFSET_FROM_GROUND     = vec3{0.0f, 1.5f, 0.0f};
constexpr f32  TELEPORTATION_DURATION = 1.0f;

//==============================================================================
/*static*/ EntityType Teleport::get_type_static()
{
  return EntityTypes::teleport;
}

//==============================================================================
Teleport::Teleport(vec3 position, EntityID to_teleport)
: Entity(position, 0.3f, 2.0f)
, time_remaining(TELEPORTATION_DURATION)
, entity(to_teleport)
{

}

//==============================================================================
void Teleport::post_init()
{
  EntityRegistry& ecs = GameSystem::get().get_entities();

  ecs.create_entity<Particle>
  (
    this->get_position() + OFFSET_FROM_GROUND,
    "teleport",
    6,
    this->time_remaining,
    colors::PURPLE,
    5.0f,
    64.0f
  );
}

//==============================================================================
void Teleport::update(f32 delta)
{
  time_remaining -= delta;
  if (time_remaining <= 0.0f)
  {
    // Kill ourselves, but before that spawn a flash of light
    EntityRegistry& ecs = GameSystem::get().get_entities();

    // Teleport the entity
    if (Entity* to_teleport = ecs.get_entity(this->entity))
    {
      to_teleport->set_position(this->get_position());
    }
    else
    {
      nc_warn("Trying to teleport an entity that does not exist..");
    }

    // Rip
    ecs.destroy_entity(this->get_id());
  }
}

}
