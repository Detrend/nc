// Project Nucledian Source File
#pragma once

#include <types.h>

namespace nc
{

using EntityType     = u32;
using EntityTypeMask = u64;

class EntityID
{
  friend class EntityRegistry;
  friend class Entity;
  EntityType type;
  u32        idx;
};

}

