// Project Nucledian Source File
#pragma once

// Feel free to include this inside headers.

#include <types.h>

namespace nc
{

using PickupType     = u8;
using WeaponType     = u8;
using AmmoType       = u8;
using WeaponFlags    = u32;
using ProjectileType = u8;
using EnemyType      = u8;

using ActorAnimState     = u8;
using ActorAnimStateFlag = u8;

constexpr WeaponType INVALID_WEAPON_TYPE = static_cast<WeaponType>(-1);

constexpr WeaponFlags weapon_flag(WeaponType type)
{
  return 1_u32 << type;
}

}
