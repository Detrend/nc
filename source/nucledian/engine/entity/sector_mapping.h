// Project Nucledian Source File
#pragma once

#include <engine/entity/entity_types.h>
#include <engine/map/map_types.h>

#include <math/matrix.h>

#include <vector>
#include <unordered_map>
#include <tuple>

namespace nc
{
struct MapSectors;
}

namespace nc
{

// Data structure that tracks which entity is in which sector and
// other way around.
// Useful for spatial queries of entities
struct SectorMapping
{
  SectorMapping(MapSectors& map);

  void on_map_rebuild();
  void on_entity_move(EntityID id, vec3 pos, f32 r, f32 h);
  void on_entity_destroy(EntityID id);
  void on_entity_create(EntityID id, vec3 pos, f32 r, f32 h);

  struct SectorsAndTransforms
  {
    std::vector<mat4>     transforms;
    std::vector<SectorID> sectors;
  };

  // we need the relative transform when the entity is in different
  // sector, but touches our sector through an nuclidean portal - in
  // that case we have to be able to transform the entity's position
  // into a position relative to our sector
  struct SectorsToEntities
  {
    std::vector<std::vector<mat4>>     transforms;
    std::vector<std::vector<EntityID>> entities;
  };

  using EntitiesToSectors = std::unordered_map<EntityID, SectorsAndTransforms>;

  SectorsToEntities sectors_to_entities;
  EntitiesToSectors entities_to_sectors;
  MapSectors&       map;
};

}

