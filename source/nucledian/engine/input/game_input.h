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
    // - //
    count
  };
}

namespace PlayerAnalogInputs
{
  enum values
  {
    look_horizontal,
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

struct GameSpecificInputs
{
  
};

struct GameInputs
{
  PlayerSpecificInputs player_inputs;
  GameSpecificInputs   game_inputs;
};

}

