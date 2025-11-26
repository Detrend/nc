// Project Nucledian Source File
#include <common.h>

#include <engine/entity/sector_mapping.h>
#include <engine/map/map_system.h>
#include <engine/entity/entity_type_definitions.h>

#include <iterator>

namespace nc
{

//==============================================================================
SectorMapping::SectorMapping(MapSectors& m)
: map(m)
{
  
}

//==============================================================================
void SectorMapping::on_map_rebuild()
{
  sectors_to_entities.entities.clear();
  sectors_to_entities.transforms.clear();
  entities_to_sectors.clear();

  sectors_to_entities.entities.resize(map.sectors.size());
  sectors_to_entities.transforms.resize(map.sectors.size());
}

//==============================================================================
void SectorMapping::on_entity_move(EntityID id, vec3 pos, f32 r, f32 h)
{
  // TODO: dumb implementation - we can track in intersection in which sectors
  //       the entity should be and add it
  this->on_entity_destroy(id);
  this->on_entity_create(id, pos, r, h);
}

//==============================================================================
void SectorMapping::on_entity_garbaged(EntityID /*id*/)
{

}

//==============================================================================
void SectorMapping::on_entity_destroy(EntityID id)
{
  auto it = entities_to_sectors.find(id);
  if (it == entities_to_sectors.end())
  {
    // entity was not in the list in the first place
    return;
  }

  SectorsAndTransforms& sector_list = it->second;
  for (auto sector_id : sector_list.sectors)
  {
    nc_assert(sector_id < sectors_to_entities.entities.size());

    auto& transform_list = sectors_to_entities.transforms[sector_id];
    auto& entity_list    = sectors_to_entities.entities[sector_id];

    u64 size = transform_list.size();
    nc_assert(size, "Can't be empty, the entity must be there");

    [[maybe_unused]]bool removed_at_least_one = false;

    u64 index = size;
    while (index-->0)
    {
      // the sector might contain the entity multiple times if it
      // is half-way in an nuclidean portal
      if (entity_list[index] == id)
      {
        // remove the entity from here
        auto e_it = std::next(entity_list.begin(),    index);
        auto t_it = std::next(transform_list.begin(), index);
        
        entity_list.erase(e_it);
        transform_list.erase(t_it);

        removed_at_least_one = true;
      }
    }

    // check here
    nc_assert(removed_at_least_one);
  }

  entities_to_sectors.erase(it);
}

//==============================================================================
void SectorMapping::on_entity_create(EntityID id, vec3 pos, f32 rad, f32 /*h*/)
{
  nc_assert(!entities_to_sectors.contains(id), "Duplicate entry!!!");

  SectorSet sectors;
  if (id.type == EntityTypes::point_light)
  {
    map.query_nearby_sectors_for_lights(pos.xz(), rad, sectors);
  }
  else
  {
    map.query_nearby_sectors_short_distance(pos.xz(), rad, sectors);
  }

  // Emplace empty
  auto[it, ok] = entities_to_sectors.emplace(id, SectorsAndTransforms{});
  nc_assert(ok);

  SectorsAndTransforms& entity_sectors = it->second;

  for (u64 i = 0, cnt = sectors.sectors.size(); i < cnt; ++i)
  {
    const mat4& transform = sectors.transforms[i];
    const SectorID sector = sectors.sectors[i];

    entity_sectors.sectors.push_back(sector);
    entity_sectors.transforms.push_back(transform);

    sectors_to_entities.entities[sector].push_back(id);
    sectors_to_entities.transforms[sector].push_back(transform);
  }

  // This is the old sector mapping that maps the entity only to one sector - the
  // one its pivot is inside of. Use this for debugging or if the above version
  // fucks up.
  /*
  SectorID sector = map.get_sector_from_point(pos.xz());
  if (sector != INVALID_SECTOR_ID)
  {
    entities_to_sectors.emplace
    (
      id,
      SectorsAndTransforms{.transforms = {mat4{1.0f}}, .sectors = {sector}}
    );

    sectors_to_entities.entities[sector].push_back(id);
    sectors_to_entities.transforms[sector].push_back(mat4{1.0f});
  }
  */
}

}



