// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <math/vector.h>

#include <array>

namespace nc
{
  inline constexpr size_t MAX_PLAYER_COUNT = 8;
  inline constexpr f32    MULTIPLAYER_FIXED_DELTA_TIME = 1.0f / 60.0f;
  // Number of frames it takes to apply player input. Longer delay hides more latency.
  inline constexpr u8     INPUT_DELAY = 2;

  using PlayerArray = std::array<struct EntityID, MAX_PLAYER_COUNT>;
  using InputArray = std::array<struct PlayerSpecificInputs, MAX_PLAYER_COUNT>;
  using PositionArray = std::array<vec3, MAX_PLAYER_COUNT>;

  using PlayerID = u8;
  inline constexpr PlayerID INVALID_PLAYER_ID = 255;

  static_assert(MAX_PLAYER_COUNT < INVALID_PLAYER_ID);
  static_assert(INPUT_DELAY >= 1);
}
