// Project Nucledian Source File
#include <engine/entity/item.h>

#include <engine/graphics/graphics_system.h>
#include <engine/core/engine.h>
#include <engine/player/player.h>

#include <engine/entity/entity_type_definitions.h>

namespace nc
{

//==============================================================================
PickUp::PickUp(vec3 position) : Entity(position, 0.15f, 0.2f, true)
{
  appear = Appearance
  {
    .texture = TextureManager::instance().get_medkit_texture(),
    .scale   = 1.0f,
  };
}

//==============================================================================
EntityType PickUp::get_type_static()
{
  return EntityTypes::pickup;
}

//==============================================================================
void PickUp::on_pickup([[maybe_unused]]const Player& player)
{
  
}

//==============================================================================
const Appearance& PickUp::get_appearance() const
{
  return const_cast<PickUp*>(this)->get_appearance();
}

//==============================================================================
Appearance& PickUp::get_appearance()
{
  return appear;
}

}
