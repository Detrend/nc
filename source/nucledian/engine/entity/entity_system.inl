// Project Nucledian Source File

#include <common.h>

namespace nc
{

//==============================================================================
template<typename L>
void EntityRegistry::for_each(EntityTypeMask types, L lambda)
{
  // iterate all pools
  for (EntityType t = 0; t < m_Pools.size(); ++t)
  {
    // and check if the pool stores the type we are interested in
    if (types & (u64{1} << t))
    {
      // if so then iterate all entities
      for (auto&[idx, entity_unique] : m_Pools[t])
      {
        lambda(*entity_unique);
      }
    }
  }
}

//==============================================================================
template<typename T, typename...Args>
T* EntityRegistry::create_entity(Args...args)
{
  nc_assert(T::get_type_static() < m_Pools.size());

  EntityID id;
  id.type = T::get_type_static();
  id.idx  = m_NextID++;

  auto[it, is_ok] = m_Pools[T::get_type_static()].insert
  (
    { id.idx, std::make_unique<T>(std::forward<Args>(args)...) }
  );

  nc_assert(is_ok);
  Entity* entity = it->second.get();
  entity->m_IdAndType = id;

  return static_cast<T*>(entity);
}

}
