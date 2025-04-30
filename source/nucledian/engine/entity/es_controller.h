// Project Nucledian Source File
#pragma once

#include <engine/entity/es_pool.h>

namespace nc
{

struct IEntityController
{
  virtual ~IEntityController(){};
};
  
template<typename T, typename...CompTypes>
struct EntityControllerHelper : public IEntityController , public CompTypes::ComponentIface...
{
  using PoolType = EntityPool<EntityID, Entity, CompTypes...>;
  virtual ~EntityControllerHelper(){};
};

}

