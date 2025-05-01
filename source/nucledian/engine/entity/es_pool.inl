// Project Nucledian Source File
#pragma once

#include <engine/entity/es_pool.h>
#include <common.h>

#include <iterator>

namespace nc
{ 
  
//==============================================================================
template<typename T>
Entity* EntityPool<T>::get_entity_idx(u64 idx)
{
  NC_ASSERT(idx < this->get_count());
  return std::next(m_Entities.begin(), idx)->second.get();
}

//==============================================================================
template<typename T>
u64 EntityPool<T>::get_count()
{
  return m_Entities.size();
}

//==============================================================================
template<typename T>
void EntityPool<T>::destroy_entity(EntityID id)
{
  if (auto it = m_Entities.find(id.idx); it != m_Entities.end())
  {
    m_Entities.erase(it);
  }
}

//==============================================================================
template<typename T>
Entity* EntityPool<T>::create_entity()
{
  const u32 idx  = m_LastIdx++;
  auto [it, okk] = m_Entities.insert({idx, std::make_unique<T>()});
  NC_ASSERT(okk);

  auto* entity = it->second.get();
  entity->id = EntityID{.idx = idx, .type = T::TYPE};
  return entity;
}

//==============================================================================
template<typename T>
Entity* EntityPool<T>::get_entity(EntityID id)
{
  if (auto it = m_Entities.find(id.idx); it != m_Entities.end())
  {
    return it->second.get();
  }

  return nullptr;
}

}

