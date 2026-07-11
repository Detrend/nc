// Project Nuclidean Source File
#pragma once

// Feel free to include this inside headers.

#include <types.h>
#include <token.h>
#include <common.h>

namespace nc
{

using PickupType     = u8;
using WeaponType     = u8;
using AmmoType       = u8;
using WeaponFlags    = u32;
using ProjectileType = u8;
using EnemyType      = Token;

using ActorAnimState     = u8;
using ActorAnimStateFlag = u8;

constexpr WeaponType INVALID_WEAPON_TYPE = cast<WeaponType>(-1);
constexpr EnemyType  INVALID_ENEMY_TYPE  = EMPTY_TOKEN;

constexpr WeaponFlags weapon_flag(WeaponType type)
{
  return 1_u32 << type;
}

}
