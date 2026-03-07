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
    particle,
		sound_emitter,
    // - //
    count,  // <- total number of entity types
    all = EntityType(-1),
  };
}

namespace SectorSnapTypes
{
  enum evalue : SectorSnapType
  {
    free = 0, // Do not snap to anything
    floor,    // Entity changes height with the floor
    ceiling,  // Entity changes height with the ceiling
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
  "particle",
	"sound_emitter",
};
static_assert(ARRAY_LENGTH(ENTITY_TYPE_NAMES) == EntityTypes::count);

constexpr vec4 ENTITY_TYPE_COLORS[]
{
	colors::WHITE,  // player
	colors::ORANGE, // enemy
	colors::GREEN,  // pickup
	colors::TEAL,   // projectile
	colors::BLACK,  // ambient_light (we don't need to see them)
	colors::BLACK,  // directional_light (we don't need to see them)
	colors::YELLOW, // point_light
	colors::PINK,   // prop
	colors::BLACK,  // sky_box (we don't need to see them)
	colors::BLUE,   // particle
	colors::RED,    // sound emitter
};
static_assert(ARRAY_LENGTH(ENTITY_TYPE_COLORS) == EntityTypes::count);

namespace EntityTypeFlags
{
  enum evalue : EntityTypeMask
  {
    player            = entity_type_to_mask(EntityTypes::player),
    enemy             = entity_type_to_mask(EntityTypes::enemy),
    pickup            = entity_type_to_mask(EntityTypes::pickup),
    projectile        = entity_type_to_mask(EntityTypes::projectile),
    ambient_light     = entity_type_to_mask(EntityTypes::ambient_light),
    directional_light = entity_type_to_mask(EntityTypes::directional_light),
    point_light       = entity_type_to_mask(EntityTypes::point_light),
    prop              = entity_type_to_mask(EntityTypes::prop),
    sky_box           = entity_type_to_mask(EntityTypes::sky_box),
    particle          = entity_type_to_mask(EntityTypes::particle),
    sound_emitter     = entity_type_to_mask(EntityTypes::sound_emitter),
  };
}
static_assert(EntityTypes::count < 64);

}

