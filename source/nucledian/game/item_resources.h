// Project Nucledian Source File
#pragma once

#include <game/game_types.h>
#include <engine/sound/sound_types.h>

// Defined as (name, sfx)
#define NC_PICKUPS(xx)     \
  xx(hp_small,     pickup) \
  xx(hp_big,       pickup) \
  xx(shotgun,      pickup) \
  xx(plasma_rifle, pickup) \
  xx(nail_gun,     pickup) \
  xx(shotgun_ammo, pickup) \
  xx(plasma_ammo,  pickup) \
  xx(nail_ammo,    pickup)


namespace nc
{

#define NC_DEFINE_PICKUP_ENUM(_item, _snd) _item,
#define NC_DEFINE_PICKUP_NAME(_item, _snd) #_item,

namespace PickupTypes
{

enum evalue : PickupType
{
  NC_PICKUPS(NC_DEFINE_PICKUP_ENUM)
  count,
  first_weapon = shotgun,
  last_weapon  = nail_gun,
  first_ammo   = shotgun_ammo,
  last_ammo    = nail_ammo,
};

}

// Defined in .cpp so we do not need to include "sound_resources.h"
extern SoundID PICKUP_SOUNDS[];

constexpr cstr PICKUP_NAMES[]
{
  NC_PICKUPS(NC_DEFINE_PICKUP_NAME)
};

}
