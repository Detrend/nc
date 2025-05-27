// Project Nucledian Source File

#include <common.h>

#include <engine/entity/entity_system.h>

#include <engine/entity/entity_type_definitions.h>
#include <engine/entity/entity.h>
#include <engine/entity/sector_mapping.h>

namespace nc
{

//==============================================================================
EntityRegistry::EntityRegistry(SectorMapping& map)
: m_Mapping(map)
{
  static_assert(MAX_POOL_CNT >= EntityTypes::count);
}

//==============================================================================
EntityRegistry::~EntityRegistry()
{
  
}

//==============================================================================
void EntityRegistry::destroy_entity(EntityID id)
{
  nc_assert(id.type < EntityTypes::count);
  m_pending_for_destruction.push_back(id);
}

//==============================================================================
const SectorMapping& EntityRegistry::get_mapping()
{
  return m_Mapping;
}

//==============================================================================
void EntityRegistry::cleanup()
{
  for (auto to_destroy : m_pending_for_destruction)
  {
    this->destroy_entity_internal(to_destroy);
  }
}

//==============================================================================
void EntityRegistry::setup_entity(Entity& entity, EntityID id)
{
  entity.m_IdAndType = id;
  entity.m_Mapping   = &m_Mapping;

  m_Mapping.on_entity_create
  (
    id, entity.get_position(), entity.get_radius(), entity.get_height()
  );
}

//==============================================================================
void EntityRegistry::destroy_entity_internal(EntityID id)
{
  nc_assert(id.type < EntityTypes::count);

  Pool& pool = m_Pools[id.type];
  auto it = pool.find(id.idx);
  if (it != pool.end())
  {
    m_Mapping.on_entity_destroy(id);

    // destroys the entity
    pool.erase(it);
  }
}

//==============================================================================
Entity* EntityRegistry::get_entity(EntityID id)
{
  if (id == INVALID_ENTITY_ID)
  {
    return nullptr;
  }

  nc_assert(id.type < EntityTypes::count);

  Pool& pool = m_Pools[id.type];
  if (auto it = pool.find(id.idx); it != pool.end())
  {
    return it->second.get();
  }

  return nullptr;
}

//==============================================================================
const Entity* EntityRegistry::get_entity(EntityID id) const
{
  return const_cast<EntityRegistry*>(this)->get_entity(id);
}

}

#ifdef NC_TESTS
#include <engine/entity/entity_tests.inl>
#endif

