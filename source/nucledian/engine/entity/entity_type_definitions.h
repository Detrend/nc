// Project Nucledian Source File
#pragma once

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
    // - //
    count  // <- total number of entity types, do not put any type
           //    after this one
  };
}

namespace EntityTypeFlags
{
  enum evalue : EntityTypeMask
  {
    player     = 1 << EntityTypes::player,
    enemy      = 1 << EntityTypes::enemy,
    pickup     = 1 << EntityTypes::pickup,
    projectile = 1 << EntityTypes::projectile,
  };
}

}

