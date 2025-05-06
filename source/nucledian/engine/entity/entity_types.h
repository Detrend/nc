// Project Nucledian Source File
#pragma once

#include <types.h>

namespace nc
{

using EntityType     = u32;
using EntityTypeMask = u64;

struct EntityID
{
  constexpr bool operator==(const EntityID& other) const = default;

  EntityType type;
  u32        idx;
};

constexpr EntityID INVALID_ENTITY_ID = EntityID{.type = ~0ul, .idx = ~0ul};

}

