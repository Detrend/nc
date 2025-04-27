// Project Nucledian Source File
#pragma once

#include <common.h>

#include <engine/map/map_types.h>
#include <math/vector.h>

#include <span>
#include <unordered_map>
#include <memory>
#include <tuple>
#include <vector>

namespace nc
{

using ComponentID = u8;
namespace Components
{
  enum evalue : ComponentID
  {
    entity_data = 0,
    render,
    physics,
    ai,
    // 
    count,
  };
}

using ComponentMask = u32;
namespace ComponentMaskTypes
{
  enum evalue : ComponentMask
  {
    entity_data = 1 << Components::entity_data,
    render      = 1 << Components::render,
    physics     = 1 << Components::physics,
    ai          = 1 << Components::ai,
  };
}

template<typename...All>
struct ComponentTypesToFlags;

template<typename First, typename...Other>
struct ComponentTypesToFlags<First, Other...>
{
  static constexpr ComponentMask value = (1 << First::COMPONENT_TYPE) | ComponentTypesToFlags<Other...>::value;
};

template<>
struct ComponentTypesToFlags<>
{
  static constexpr ComponentMask value = 0;
};

struct IEntityController
{
  virtual ~IEntityController(){};
};

struct Entity;
struct AIComp;
struct RenderComp;
struct PhysComp;
struct EntityID;
struct IEntityPool;

struct IEntityIface
{
  virtual ~IEntityIface(){};
};

struct IRenderIface
{
  virtual void on_render()
  {
    
  }

  RenderComp* get_render()
  {
    return nullptr;
  }

  virtual ~IRenderIface(){};
};

struct PhysIface
{
  virtual void on_collision(const EntityID& /*other*/)
  {
    
  }

  PhysComp* get_physics()
  {
    return nullptr;
  }

  virtual ~PhysIface(){};
};

struct AIIface
{
  virtual ~AIIface(){};
};

struct EntityID
{
  static constexpr ComponentMask COMPONENT_TYPE = 0;

  ComponentMask pool;
  u32          id;
};

template<ComponentID ID, typename CompIface>
struct ComponentHelper
{
  static constexpr ComponentID COMPONENT_TYPE = ID;
  using ComponentIface = CompIface;

  IEntityPool* pool = nullptr;
  EntityID     parent_id;
};

struct Entity : ComponentHelper<Components::entity_data, IEntityIface>
{
  vec3                               position;
  std::unique_ptr<IEntityController> controller;

  //virtual ~Entity(){};
};

struct IEntityPool
{
  virtual EntityID create_entity() = 0;

  virtual void     destroy_entity_id(EntityID id) = 0;
  virtual void*    get_component_id(EntityID id, ComponentID type) = 0;

  virtual void*    get_component_idx(u64 entity_idx, ComponentID component) = 0;

  virtual u64      get_count() const = 0;

  virtual ~IEntityPool(){};
};

struct RenderComp : ComponentHelper<Components::render, IRenderIface>
{
  u64 num = 0;

  #pragma optimize("", off)
  inline const AIComp* get_ai() const
  {
    return static_cast<AIComp*>(pool->get_component_id(parent_id, Components::ai));
  }

  inline u64 get_num() const
  {
    return num;
  }
  #pragma optimize("", on)
};

struct PhysComp : ComponentHelper<Components::physics, PhysIface>
{
  
};

struct AIComp : ComponentHelper<Components::ai, AIIface>
{
  
};

template<typename...CompTypes>
class EntityPool : public IEntityPool
{
public:
  static constexpr auto POOL_ID = ComponentTypesToFlags<CompTypes...>::value;

  virtual EntityID create_entity() override
  {
    const EntityID identifier
    {
      .pool = POOL_ID,
      .id   = m_LastId++,
    };

    m_IndexMapping[identifier.id] = static_cast<u32>(get_count());
    this->for_each([&]<typename T>(std::vector<T>& vec)
    {
      vec.emplace_back();
      if constexpr (!std::is_same_v<T, EntityID>)
      {
        vec.back().parent_id = identifier;
        vec.back().pool      = this;
      }
      else
      {
        vec.back() = identifier;
      }
    });

    return identifier;
  }

  virtual void* get_component_idx(u64 idx, ComponentID type) override
  {
    void* comp_ptr = nullptr;
    NC_ASSERT(idx < std::get<0>(m_Components).size());

    this->for_each([&]<typename T>(std::vector<T>& vec)
    {
      if (T::COMPONENT_TYPE == type)
      {
        comp_ptr = &vec[idx];
      }
    });

    return comp_ptr;
  }

  void* get_component_id(EntityID id, ComponentID type) override
  {
    if (m_IndexMapping.contains(id.id))
    {
      const auto index = m_IndexMapping[id.id];
      return this->get_component_idx(index, type);
    }
    else
    {
      return nullptr;
    }
  }

  void destroy_entity_id(EntityID id) override
  {
    if (m_IndexMapping.contains(id.id))
    {
      const u32      idx     = m_IndexMapping[id.id];
      const EntityID last_id = std::get<std::vector<EntityID>>(m_Components).back();

      this->destroy_entity_idx(idx);

      m_IndexMapping.erase(id.id);

      if (this->get_count())
      {
        m_IndexMapping[last_id.id] = idx;
      }
    }
  }

  u64 get_count() const override
  {
    return std::get<0>(m_Components).size();
  }

private:
  void destroy_entity_idx(u32 idx)
  {
    const auto size = this->get_count();
    NC_ASSERT(idx < size);

    this->for_each([&](auto& vec)
    {
      std::swap(vec[idx], vec[size-1]);
      vec.pop_back();
    });
  }

  template<typename L>
  void for_each(L lambda)
  {
    (this->for_one<CompTypes>(lambda), ...);
  }

  template<typename T, typename L>
  void for_one(L lambda)
  {
    lambda(std::get<std::vector<T>>(m_Components));
  }

private:
  template<typename T>
  using column = std::vector<T>;

  std::tuple<column<CompTypes>...> m_Components;
  std::unordered_map<u32, u32>     m_IndexMapping;
  u32                              m_LastId = 0;
};

template<typename...T>
struct PoolType
{
  using type = EntityPool<EntityID, Entity, T...>;
};


template<typename T, typename...CompTypes>
struct EntityControllerHelper : public IEntityController, public CompTypes::ComponentIface...
{
  using ControlerPoolType = typename PoolType<CompTypes...>::type;

  virtual ~EntityControllerHelper(){};
};


template<typename Component>
Component& get_component_from_pool(IEntityPool& pool, u64 entity_idx)
{
  return *static_cast<Component*>(pool.get_component_idx(entity_idx, Component::COMPONENT_TYPE));
}


class EntitySystem
{ 
public:
  template<typename...CompTypes>
  EntityID create_entity_pure()
  {
    using PT = typename PoolType<CompTypes...>::type;
    constexpr ComponentMask pool_id = PT::POOL_ID;

    if (!m_Pools.contains(pool_id)) [[unlikely]]
    {
      m_Pools[pool_id] = std::unique_ptr<IEntityPool>(new PT());
    }

    return m_Pools[pool_id]->create_entity();
  }

  template<typename Controller>
  EntityID create_entity_controled()
  {
    using PT = typename Controller::ControlerPoolType;
    constexpr ComponentMask pool_id = PT::POOL_ID;

    if (!m_Pools.contains(pool_id)) [[unlikely]]
    {
      m_Pools[pool_id] = std::unique_ptr<IEntityPool>(new PT());
    }

    auto id = m_Pools[pool_id]->create_entity();
    auto* entity = this->get_component<Entity>(id);
    NC_ASSERT(entity);
    entity->controller = std::make_unique<Controller>();

    return id;
  }

  void destroy_entity(EntityID id)
  {
    if (m_Pools.contains(id.pool))
    {
      m_Pools[id.pool]->destroy_entity_id(id);
    }
  }

  template<typename T>
  T* get_component(EntityID entity)
  {
    if (m_Pools.contains(entity.pool))
    {
      return static_cast<T*>(m_Pools[entity.pool]->get_component_id(entity, T::COMPONENT_TYPE));
    }

    return nullptr;
  }

  // Iterates all entities with given components and calls the lambda argument
  // with references to them.
  // Lambda has signature void(IEntity&, ComponentTypes&...)
  template<typename...ComponentTypes, typename Lambda>
  void for_each_entity(Lambda lambda)
  {
    const ComponentMask flags = ComponentTypesToFlags<ComponentTypes...>::value;

    for (auto&[pool_flags, pool] : m_Pools)
    {
      if ((pool_flags & flags) != flags)
      {
        continue;
      }

      for (u64 idx = 0; idx < pool->get_count(); ++idx)
      {
        lambda(get_component_from_pool<ComponentTypes>(*pool, idx)...);
      }
    }
  }

private:
  using Pools = std::unordered_map<ComponentMask, std::unique_ptr<IEntityPool>>;
  Pools m_Pools;
};
  
}


