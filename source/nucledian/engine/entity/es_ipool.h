// Project Nucledian Source File
#pragma once
#include <types.h>
#include <engine/entity/es_types.h>

#include <span>

namespace nc
{

struct IEntityPool
{
  virtual EntityID create_entity() = 0;
  virtual void     destroy_entity_id(std::span<EntityID> ids) = 0;
  virtual void*    get_component_id(EntityID id, ComponentID type) = 0;
  virtual void*    get_component_idx(u64 entity_idx, ComponentID component) = 0;
  virtual u64      get_count() const = 0;

  virtual ~IEntityPool(){};
};

}


