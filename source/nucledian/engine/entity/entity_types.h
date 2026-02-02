// Project Nucledian Source File
#pragma once

#include <types.h>
#include <vector> // required for std::hash specialization
#include <common.h>

namespace nc
{

using EntityType     = u8;
using EntityTypeMask = u64;
using SectorSnapType = u8;

struct EntityID
{
  constexpr bool operator==(const EntityID& other) const = default;

  EntityType type;
  u32        idx;

  // Converts to u64 so it can be treated as a normal integer.
  u32 as_u32() const
  {
    nc_assert(idx <= (1 << 24) - 1);
    
    return (static_cast<u32>(idx) << 24) | static_cast<u32>(type);
    // return (u64{ type } << 24) | u64{ idx };
  }
};
static_assert(sizeof(EntityID) == 8);

constexpr EntityID INVALID_ENTITY_ID = EntityID
{
  .type = std::numeric_limits<EntityType>().max(),
  .idx  = std::numeric_limits<u32>().max(),
 };

constexpr EntityTypeMask entity_type_to_mask(EntityType type)
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

