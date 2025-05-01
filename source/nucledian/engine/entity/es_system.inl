// Project Nucledian Source File
#pragma once

#include <common.h>

namespace nc
{
  
//==============================================================================
template<typename T, typename L>
void EntitySystem::for_each_entity(L lambda)
{
  if (auto it = m_Pools.find(T::TYPE); it != m_Pools.end())
  {
    const u64 cnt = it->second->get_count();
    for (u64 i = 0; i < cnt; ++i)
    {
      auto* ent = static_cast<T*>(it->second->get_entity_idx(i));
      lambda(*ent);
    }
  }
}

//==============================================================================
template<typename T>
T* EntitySystem::create_entity()
{
  auto it = m_Pools.find(T::TYPE);

  if (it == m_Pools.end())
  {
    auto[i, okk] = m_Pools.insert({T::TYPE, std::make_unique<EntityPool<T>>()});
    NC_ASSERT(okk);
    it = i;
  }

  return static_cast<T*>(it->second->create_entity());
}

}

