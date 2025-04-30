// Project Nucledian Source File
#pragma once

#include <engine/entity/es_types.h>
#include <engine/entity/es_ipool.h> // nc::IEntityPool

#include <unordered_map>  // std::unordered_map
#include <memory>         // std::unique_ptr

namespace nc
{

class EntitySystem
{ 
public:
  template<typename...CompTypes>
  EntityID create_entity_pure();

  template<typename Controller>
  EntityID create_entity_controlled();

  void destroy_entity(EntityID id);

  template<typename T>
  T* get_component(EntityID entity);

  // Iterates all entities with given components and calls the lambda argument
  // with references to them.
  // Lambda has signature void(IEntity&, ComponentTypes&...)
  template<typename...ComponentTypes, typename Lambda>
  void for_each_entity(Lambda lambda);

private:
  template<typename PoolType>
  IEntityPool* get_or_create_pool();

private:
  using Pools = std::unordered_map<ComponentMask, std::unique_ptr<IEntityPool>>;
  Pools m_Pools;
};

}

#include <engine/entity/es_system.inl>
