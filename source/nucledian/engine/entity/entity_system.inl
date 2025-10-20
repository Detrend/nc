// Project Nucledian Source File

#include <common.h>

namespace nc
{

//==============================================================================
template<typename T>
T* EntityRegistry::get_entity(EntityID id)
{
  nc_assert(id == INVALID_ENTITY_ID || id.type == T::get_type_static());
  return static_cast<T*>(this->get_entity(id));
}

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
template<typename T, typename L>
void EntityRegistry::for_each(L lambda)
{
  this->for_each(EntityTypeToMask(T::get_type_static()), [&](Entity& entity)
  {
    lambda(*static_cast<T*>(&entity));
  });
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
  this->setup_entity(*entity, id);

  return static_cast<T*>(entity);
}

}
