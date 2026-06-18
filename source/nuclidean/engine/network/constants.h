// Project Nuclidean Source File
#pragma once

#include <types.h>

#include <array>

namespace nc
{
  inline constexpr size_t g_max_player_count    = 2;
  inline constexpr f32    g_mp_fixed_delta_time = 1.0f / 60.0f;

  using PlayerArray      = std::array<struct EntityID, g_max_player_count>;
  using PlayerInputArray = std::array<struct PlayerSpecificInputs, g_max_player_count>;
}