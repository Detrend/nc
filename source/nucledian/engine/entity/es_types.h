// Project Nucledian Source File
#pragma once

#include <types.h>

namespace nc
{
  
using EntityType = u8;
namespace EntityTypes
{
  enum evalue : EntityType
  {
    player = 0,
    actor,
    enemy,
    static_geom,
    pickup,
  };
}

struct EntityID
{
  u32        idx;
  EntityType type;
  u8         padd[3];
};

struct Entity
{
  EntityID id;
};

}

