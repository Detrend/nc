// Project Nucledian Source File
#pragma once

#include <types.h>

namespace nc
{
  
using PlayerKeyFlags = u16;
namespace PlayerKeyInputs
{
  enum values : PlayerKeyFlags
  {
    forward = 0,
    backward,
    left,
    right,
    jump,

    primary,
    secondary,

    // Maps continuously to weapons in their enum..
    weapon_0, // wrench
    weapon_1, // shotgun
    weapon_2, // plasma gun
    weapon_3, // nail_gun

    // - //
    count
  };
}
static_assert
(
  PlayerKeyInputs::count <= sizeof(PlayerKeyFlags) * 8,
  "We run out of bits, increase the size of PlayerKeyFlags"
);

namespace PlayerAnalogInputs
{
  enum values
  {
    look_horizontal = 0,
    look_vertical,
    // - //
    count
  };
}

struct PlayerSpecificInputs
{
  PlayerKeyFlags keys = 0;
  f32            analog[PlayerAnalogInputs::count]{};
};

// These should contain something like "ESC to go to menu"
struct GameSpecificInputs
{
  
};

struct GameInputs
{
  PlayerSpecificInputs player_inputs;
  GameSpecificInputs   game_inputs;
};

}

