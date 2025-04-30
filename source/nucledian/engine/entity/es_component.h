// Project Nucledian Source File
#pragma once

#include <engine/entity/es_types.h>
#include <engine/entity/es_ipool.h>
#include <engine/entity/es_component_types.h>

#include <math/vector.h>

#include <memory>

namespace nc
{
struct IEntityController;
}

namespace nc
{

template<ComponentID ID, typename CompIface>
struct ComponentHelper
{
  static constexpr ComponentID COMPONENT_TYPE = ID;
  using ComponentIface = CompIface;

  IEntityPool* pool = nullptr;
  EntityID     parent_id;
};

struct IEntityIface
{
  virtual ~IEntityIface(){};
};

struct Entity : ComponentHelper<Components::entity_data, IEntityIface>
{
  vec3                               position;
  std::unique_ptr<IEntityController> controller;
};

}

