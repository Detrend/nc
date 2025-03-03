#pragma once

#include <types.h>

namespace nc
{

// Determines how long a resource lives.
enum class ResLifetime : u8
{
  None,
  // Resource lives until current level is unloaded. This is typically used for level-specific assets.
  Level,
  // Resources lives until the game is terminated. This is typically used for assets that are shared across levels.
  Game,
};

}
