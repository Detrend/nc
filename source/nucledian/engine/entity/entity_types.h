// Project Nucledian Source File
#pragma once

#include <types.h>
#include <vector> // required for std::hash specialization

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

constexpr EntityTypeMask EntityTypeToMask(EntityType type)
{
  return EntityTypeMask{1} << type;
}

}

// Specialize the standard hash function so we can use EntityID in
// std::unordered_set
template<>
struct std::hash<nc::EntityID>
{
  inline std::size_t operator()(const nc::EntityID& id) const
  {
    return static_cast<nc::u64>(id.type) << 32 | static_cast<nc::u64>(id.idx);
  }
};

