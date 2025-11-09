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

  // Signature of func: void(EntityID, mat4)
  // Iterates all entities in the given sector and calls func with ID and a
  // relative transform of each one.
  template<typename F>
  void for_each_in_sector(SectorID sector, F&& func) const;

  // Signature of func: void(EntityID, mat4)
  // Same as above, but iterates only entities of the given type.
  template<typename EntityT, typename F>
  void for_each_in_sector(SectorID sector, F&& func) const;

  struct SectorsAndTransforms
  {
    std::vector<mat4>     transforms;
    std::vector<SectorID> sectors;
  };

  // We need the relative transform when the entity is in a different sector,
  // but touches our sector through a nuclidean portal - in  that case we have
  // to be able to transform the entity's position into a position relative to
  // our sector.
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

#include <engine/entity/sector_mapping.inl>
