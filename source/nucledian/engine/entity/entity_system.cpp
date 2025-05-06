// Project Nucledian Source File

#include <common.h>

#include <engine/entity/entity_system.h>

#include <engine/entity/entity_type_definitions.h>
#include <engine/entity/entity.h>

namespace nc
{

//==============================================================================
EntityRegistry::EntityRegistry()
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

  Pool& pool = m_Pools[id.type];
  auto it = pool.find(id.idx);
  if (it != pool.end())
  {
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

}

#ifdef NC_TESTS
#include <engine/entity/entity_tests.inl>
#endif

