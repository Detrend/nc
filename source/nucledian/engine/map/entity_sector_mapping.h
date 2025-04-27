// Project Nucledian Source File

#include <types.h>
#include <engine/map/map_types.h>

#include <math/vector.h>

#include <vector>
#include <unordered_map>

namespace nc
{

struct MapSectors;

using EntityID = u64;

struct EntitySectorMapping
{
  using SectorIDs = std::vector<SectorID>;
  using EntityIDs = std::vector<EntityID>;

  using SectorsToEntityIDs   = std::vector<EntityIDs>;
  using EntityIDsToSectorIDs = std::unordered_map<EntityID, SectorIDs>;

  void on_entity_created(EntityID id, vec3 position, f32 size);
  void on_entity_destroyed(EntityID id);
  void on_entity_moved(EntityID id, vec3 from, vec3 to, f32 size);

  SectorsToEntityIDs   sectors_to_entities;
  EntityIDsToSectorIDs entities_to_sectors;
  MapSectors*          map;
};

}