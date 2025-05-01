// Project Nucledian Source File
#pragma once

#include <engine/entity/es_types.h>

#include <unordered_map>
#include <memory>

namespace nc
{

struct IEntityPool
{
  virtual Entity* get_entity(EntityID id)     = 0;
  virtual Entity* create_entity()             = 0;
  virtual void    destroy_entity(EntityID id) = 0;
  virtual u64     get_count()                 = 0;
  virtual Entity* get_entity_idx(u64 idx)     = 0;

  virtual ~IEntityPool(){};
};

template<typename T>
struct EntityPool : IEntityPool
{
  Entity* get_entity(EntityID id)     override;
  Entity* create_entity()             override;
  void    destroy_entity(EntityID id) override;
  u64     get_count()                 override;
  Entity* get_entity_idx(u64 idx)     override;

  // MR says: this is just a prototype version, can be done
  // better using a vector
  std::unordered_map<u32, std::unique_ptr<T>> m_Entities;
  u32                                         m_LastIdx = 0;
};

}

#include <engine/entity/es_pool.inl>

