// Project Nucledian Source File
#pragma once

#include <engine/entity/es_system.h>
#include <engine/entity/es_pool.h>
#include <engine/entity/es_component.h>

namespace nc
{

//==============================================================================
// Returns the type of the pool the entity with these components
// belongs in
template<typename...T>
struct PoolType
{
  using type = EntityPool<EntityID, Entity, T...>;
};


//==============================================================================
template<typename Component>
Component& get_component_from_pool(IEntityPool& pool, u64 entity_idx)
{
  return *static_cast<Component*>(pool.get_component_idx(entity_idx, Component::COMPONENT_TYPE));
};

//==============================================================================
template<typename...CompTypes>
EntityID EntitySystem::create_entity_pure()
{
  auto* pool = this->get_or_create_pool<typename PoolType<CompTypes...>::type>();
  return pool->create_entity();
}

//==============================================================================
template<typename Controller>
EntityID EntitySystem::create_entity_controlled()
{
  auto* pool   = this->get_or_create_pool<typename Controller::PoolType>();
  auto  id     = pool->create_entity();
  auto* entity = this->get_component<Entity>(id);

  NC_ASSERT(entity);
  entity->controller = std::make_unique<Controller>();

  return id;
}

//==============================================================================
template<typename T>
T* EntitySystem::get_component(EntityID entity)
{
  if (m_Pools.contains(entity.pool))
  {
    void* ptr = m_Pools[entity.pool]->get_component_id(entity, T::COMPONENT_TYPE);
    NC_ASSERT(ptr);
    return static_cast<T*>(ptr);
  }

  return nullptr;
}

//==============================================================================
template<typename...ComponentTypes, typename Lambda>
void EntitySystem::for_each_entity(Lambda lambda)
{
  const ComponentMask flags = ComponentTypesToFlags<ComponentTypes...>::value;

  for (auto&[pool_flags, pool] : m_Pools)
  {
    if ((pool_flags & flags) != flags)
    {
      // this pool does not have all the desired components
      continue;
    }

    for (u64 idx = 0; idx < pool->get_count(); ++idx)
    {
      lambda(get_component_from_pool<ComponentTypes>(*pool, idx)...);
    }
  }
}

//==============================================================================
template<typename PoolType>
IEntityPool* EntitySystem::get_or_create_pool()
{
  constexpr ComponentMask mask = PoolType::POOL_ID;
  auto it = m_Pools.find(mask);

  if (it == m_Pools.end())
  {
    auto[i, is_ok] = m_Pools.insert({mask, std::make_unique<PoolType>()});
    NC_ASSERT(is_ok);
    it = i;
  }

  return it->second.get();
}

}


