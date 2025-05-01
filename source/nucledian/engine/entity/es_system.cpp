// Project Nucledian Source File
#include <engine/entity/es_system.h>

#include <span>

namespace nc
{

//==============================================================================
void EntitySystem::destroy_entity(EntityID id)
{
  if (m_Pools.contains(id.pool))
  {
    m_Pools[id.pool]->destroy_entity_id(std::span{&id, &id+1});
  }
}
  
}

