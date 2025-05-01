// Project Nucledian Source File
#pragma once

#include <engine/entity/es_pool.h>

#include <tuple>

namespace nc
{

struct IEntityController
{
  virtual ~IEntityController(){};
};

template<typename...T>
struct ExpandPool;

template<typename...BaseTypes, typename...NewTypes>
struct ExpandPool<EntityPool<EntityID, Entity, BaseTypes...>, NewTypes...>
{
  using type = EntityPool<EntityID, Entity, BaseTypes..., NewTypes...>;
};
  
template<typename...CompTypes>
struct EntityControllerHelper : public IEntityController , public CompTypes::ComponentIface...
{
  using PoolType = EntityPool<EntityID, Entity, CompTypes...>;
  virtual ~EntityControllerHelper(){};
};

template<typename Base, typename...CompTypes>
struct EntityControllerDerived : public CompTypes::ComponentIface..., public Base
{
  using PoolType = typename ExpandPool<typename Base::PoolType, CompTypes...>::type;
  virtual ~EntityControllerDerived(){};
};

}

