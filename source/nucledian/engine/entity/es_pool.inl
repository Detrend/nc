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
  return &m_Entities[idx];
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
  auto it = m_Mapping.find(id.idx);
  if (it == m_Mapping.end())
  {
    return;
  }

  const u32 idx      = it->second;
  const u32 last_idx = static_cast<u32>(m_Entities.size() - 1);

  // edge cases:
  // - the removed entity is the last entity
  // - the removed entity is on the last position

  if (idx == last_idx)
  {
    // this happens if we are the last entity in the vector
    // or there is only one entity - us
    m_Entities.pop_back();
    m_Mapping.erase(it);
  }
  else
  {
    std::swap(m_Entities[idx], m_Entities[last_idx]);
    m_Entities.pop_back();
    m_Mapping.erase(it);
    m_Mapping[m_Entities[idx].id.idx] = idx;
  }
}

//==============================================================================
template<typename T>
Entity* EntityPool<T>::create_entity()
{
  const u32 id = m_LastIdx++;
  auto& entity = m_Entities.emplace_back();

  // Set the ID
  entity.id = EntityID{.idx = id, .type = T::TYPE};

  // Set the correct mapping
  m_Mapping[id] = static_cast<u32>(m_Entities.size() - 1);

  return &entity;
}

//==============================================================================
template<typename T>
Entity* EntityPool<T>::get_entity(EntityID id)
{
  if (auto it = m_Mapping.find(id.idx); it != m_Mapping.end())
  {
    return &m_Entities[it->second];
  }

  return nullptr;
}

}

