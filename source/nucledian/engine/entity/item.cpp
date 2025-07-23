#include "item.h"
#include <engine/graphics/graphics_system.h>
#include <engine/core/engine.h>

#include <engine/entity/entity_type_definitions.h>

namespace nc
{
  PickUp::PickUp(vec3 position) : Entity(position, 0.15f, 0.2f, true)
{
  auto& gfx = get_engine().get_module<GraphicsSystem>();

  appear = Appearance
  {
    .color = colors::BLUE,
    .model = gfx.get_cube_model(),
    .transform = Transform{VEC3_ZERO, vec3{0.3f, 0.4f, 0.3f}}
  };
}

  //  ==========================================

  EntityType PickUp::get_type_static()
  {
    return EntityTypes::pickup;
  }

//=========================================================

void PickUp::on_pickup([[maybe_unused]] Player player)
{
}

const Appearance& PickUp::get_appearance() const
{
  return appear;
}
}

//==============================================


