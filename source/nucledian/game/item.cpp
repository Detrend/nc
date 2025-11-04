// Project Nucledian Source File
#include <game/item.h>
#include <cvars.h>

#include <engine/player/player.h> // for checking if should be picked up

#include <engine/graphics/resources/texture.h> // graphics
#include <engine/appearance.h>

#include <engine/entity/entity_type_definitions.h> // EntityTypes::pickup
#include <engine/sound/sound_system.h>             // Playing sounds

#include <game/item_resources.h>     // PICKUP_NAMES, PICKUP_SOUNDS

namespace nc
{

//==============================================================================
PickUp::PickUp(vec3 position, PickupType my_type)
: Entity(position, 0.15f, 0.2f, true)
, type(my_type)
{
  cstr texture_name = PICKUP_NAMES[this->type];

  appear = Appearance
  {
    .sprite = texture_name,
    .scale  = 2.0f,
    .mode   = Appearance::SpriteMode::mono,
  };
}

//==============================================================================
EntityType PickUp::get_type_static()
{
  return EntityTypes::pickup;
}

//==============================================================================
bool PickUp::pickup([[maybe_unused]]const Player& player)
{
  bool picked_up = false;

  switch (this->type)
  {
    // Medkits
    case PickupTypes::hp_small:
    case PickupTypes::hp_big:
    {
      // TODO: pick up only if the player does not have full HP already
      picked_up = true;
      Player& p = const_cast<Player&>(player);
      for (int i = 0; i < 4; ++i)
      {
        p.current_ammo[i] = min(p.current_ammo[i] + 10, Player::MAX_AMMO[i]);
      }
      
      p.current_health = min(p.current_health + 20, CVars::player_max_hp);
      break;
    }

    default:
    {
      break;
    }
  }

  if (picked_up)
  {
    SoundSystem::get().play_oneshot(PICKUP_SOUNDS[this->type]);
  }

  return picked_up;
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
