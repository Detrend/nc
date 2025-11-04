// Project Nucledian Source File
#pragma once

#include <engine/entity/entity_types.h>
#include <engine/map/map_types.h>
#include <engine/entity/entity_system_listener.h>

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
struct SectorMapping : public IEntityListener
{
  SectorMapping(MapSectors& map);

  void on_map_rebuild();

  virtual void on_entity_move(EntityID id, vec3 pos, f32 r, f32 h)   override;
  virtual void on_entity_garbaged(EntityID id)                       override;
  virtual void on_entity_destroy(EntityID id)                        override;
  virtual void on_entity_create(EntityID id, vec3 pos, f32 r, f32 h) override;

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

