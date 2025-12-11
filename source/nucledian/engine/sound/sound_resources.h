// Project Nucledian Source File
#pragma once

#include <types.h>
#include <metaprogramming.h>

#include <engine/sound/sound_types.h>

// Register your sounds here. The sound files with the same name will be loaded.
// The syntax for new sound registration is xx(name, pitch variance), where the
// first parameter is obvious and the second one is the maximum pitch change the
// sound can have (0 for none). This is not implemented for now, but will later
// allow us to randomly change a pitch of certain sounds within some bounds.
// For now we support only the .wav sound format.
#define NC_SOUNDS(xx)        \
  xx(nail_gun,     0.1f)     \
  xx(plasma_rifle, 0.1f)     \
  xx(ricochet,     0.1f)     \
  xx(pickup,       0.1f)     \
  xx(hurt,         0.1f)     \
  xx(death,        0.1f)     \
  xx(melee_hit,    0.1f)

#define NC_SOUND_DIRECTORY_CSTR "content/sound/"
#define NC_SOUND_TYPE           ".wav"
#define NC_REGISTER_SOUND_ENUM(name, _pitch_not_used) name,
#define NC_REGISTER_SOUND_PATH(name, _pitch_not_used) NC_SOUND_DIRECTORY_CSTR #name NC_SOUND_TYPE ,
#define NC_REGISTER_SOUND_PITCH(_name_not_used, pitch) pitch,

namespace nc
{

namespace Sounds
{
  enum evalue : SoundID
  {
    NC_SOUNDS(NC_REGISTER_SOUND_ENUM)
    // - //
    count
  };
}

constexpr cstr SOUND_FILES[]
{
  NC_SOUNDS(NC_REGISTER_SOUND_PATH)
};

constexpr f32 SOUND_PITCHES[]
{
  NC_SOUNDS(NC_REGISTER_SOUND_PITCH)
};

constexpr u64 TOTAL_SOUND_CNT = Sounds::count;
static_assert(ARRAY_LENGTH(SOUND_FILES)   == TOTAL_SOUND_CNT);
static_assert(ARRAY_LENGTH(SOUND_PITCHES) == TOTAL_SOUND_CNT);

}
