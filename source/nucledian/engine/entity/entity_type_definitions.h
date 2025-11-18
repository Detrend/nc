// Project Nucledian Source File
#pragma once

#include <metaprogramming.h>
#include <engine/entity/entity_types.h>

// ================================================================
// Possible entity types. We keep the types stored in a separate //
// header instead of the "entity_types.h" so that we do not      //
// need to recompile the whole codebase each time a new entity   //
// type is added - entity_types.h will be probably included in   //
// many places, but entity_type_definitions.h not so much.       //
// Therefore, try not to include this into many headers as it    //
// will then transitively get included everywhere else.          //
// ================================================================

namespace nc
{

// Add your new entity type here and into the second enum below
// as well.
namespace EntityTypes
{
  enum evalue : EntityType
  {
    player = 0,
    enemy,
    pickup,
    projectile,
    ambient_light,
    directional_light,
    point_light,
    prop,
    sky_box,
    // - //
    count  // <- total number of entity types, do not put any type
           //    after this one
  };
}

constexpr cstr ENTITY_TYPE_NAMES[]
{
  "player",
  "enemy",
  "pickup",
  "projectile",
  "ambient_light",
  "directional_light",
  "point_light",
  "prop",
  "sky_box",
};
static_assert(ARRAY_LENGTH(ENTITY_TYPE_NAMES) == EntityTypes::count);

namespace EntityTypeFlags
{
  enum evalue : EntityTypeMask
  {
    player            = EntityTypeToMask(EntityTypes::player),
    enemy             = EntityTypeToMask(EntityTypes::enemy),
    pickup            = EntityTypeToMask(EntityTypes::pickup),
    projectile        = EntityTypeToMask(EntityTypes::projectile),
    ambient_light     = EntityTypeToMask(EntityTypes::ambient_light),
    directional_light = EntityTypeToMask(EntityTypes::directional_light),
    point_light       = EntityTypeToMask(EntityTypes::point_light),
    prop              = EntityTypeToMask(EntityTypes::prop),
    sky_box           = EntityTypeToMask(EntityTypes::sky_box),
  };
}

}

