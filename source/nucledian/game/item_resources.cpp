// Project Nucledian Source File
#include <game/item_resources.h>

#include <engine/sound/sound_resources.h>

namespace nc
{

#define NC_DEFINE_PICKUP_SOUND(_item, _snd) Sounds::_snd,
SoundID PICKUP_SOUNDS[]
{
  NC_PICKUPS(NC_DEFINE_PICKUP_SOUND)
};

}
