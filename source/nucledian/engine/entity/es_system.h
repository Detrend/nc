// Project Nucledian Source File
#pragma once

#include <engine/entity/es_pool.h>
#include <engine/entity/es_types.h>

#include <types.h>

#include <memory>
#include <unordered_map>

namespace nc
{

class EntitySystem
{
public:
  Entity* get_entity(EntityID id);

  template<typename T>
  T* create_entity();

  void destroy_entity(EntityID id);

  template<typename T, typename L>
  void for_each_entity(L lambda);

private:
  using Pools = std::unordered_map<EntityType, std::unique_ptr<IEntityPool>>;
  Pools m_Pools;
};

}

#include <engine/entity/es_system.inl>
