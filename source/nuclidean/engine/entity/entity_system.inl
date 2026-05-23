// Project Nuclidean Source File

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
  for (EntityType t = 0; t < m_pools.size(); ++t)
  {
    // and check if the pool stores the type we are interested in
    if (types & (1_u64 << t))
    {
      // if so then iterate all entities
      IEntityPool* pool = m_pools[t].get();
      nc_assert(pool);

      u64   cnt    = pool->get_cnt();
      u64   stride = pool->get_stride();
      byte* ptr    = recast<byte*>(pool->get_first());

      // iterate all entities within the pool
      for (u64 i = 0; i < cnt; ++i)
      {
        Entity* entity = recast<Entity*>(&ptr[i * stride]);
        lambda(*entity);
      }
    }
  }
}

//==============================================================================
template<typename T, typename L>
void EntityRegistry::for_each(L lambda)
{
  this->for_each(entity_type_to_mask(T::get_type_static()), [&](Entity& entity)
  {
    lambda(*static_cast<T*>(&entity));
  });
}

//==============================================================================
template<typename T, typename...Args>
T* EntityRegistry::create_entity(Args&&...args)
{
  nc_assert(T::get_type_static() < m_pools.size());

  EntityID id;
  id.type = T::get_type_static();

  Entity* entity = m_pools[T::get_type_static()]->create(id.idx);

  // Setup ID
  nc_assert(entity);
  this->setup_entity(*entity, id);

  // Notice that we have to call "post_init" on the correct type as it is
  // non-virtual.
  T* entity_typed = static_cast<T*>(entity);
  entity_typed->init(std::forward<Args>(args)...);

  // Call listeners
  this->post_init_entity(*entity);

  // And call post init (not sure if needed)
  entity_typed->post_init();

  return entity_typed;
}

}
