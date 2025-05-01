// Project Nucledian Source File
#include <engine/entity/es_system.h>

#include <common.h>

namespace nc
{
  
//==============================================================================
Entity* EntitySystem::get_entity(EntityID id)
{
  if (auto it = m_Pools.find(id.type); it != m_Pools.end())
  {
    return it->second->get_entity(id);
  }
  return nullptr;
}

//==============================================================================
void EntitySystem::destroy_entity(EntityID id)
{
  if (auto it = m_Pools.find(id.type); it != m_Pools.end())
  {
    return it->second->destroy_entity(id);
  }
}

}

