// Project Nucledian Source File
#pragma once

#include <engine/entity/es_types.h>

namespace nc
{

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

    m_IndexMapping[identifier.id] = static_cast<u32>(this->get_count());
    m_Components.emplace_back();

    this->for_each(this->get_count()-1, [&]<typename T>(T& comp)
    {
      if constexpr (!std::is_same_v<T, EntityID>)
      {
        comp.parent_id = identifier;
        comp.pool      = this;
      }
      else
      {
        comp = identifier;
      }
    });

    return identifier;
  }

  virtual void* get_component_idx(u64 idx, ComponentID type) override
  {
    void* comp_ptr = nullptr;
    NC_ASSERT(idx < this->get_count());

    this->for_each(idx, [&]<typename T>(T& value)
    {
      if (T::COMPONENT_TYPE == type)
      {
        comp_ptr = &value;
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

  void destroy_entity_id(std::span<EntityID> ids) override
  {
    for (const auto id : ids)
    {
      if (m_IndexMapping.contains(id.id))
      {
        const u32      idx     = m_IndexMapping[id.id];
        const EntityID last_id = std::get<EntityID>(m_Components.back());

        this->destroy_entity_idx(idx);

        m_IndexMapping.erase(id.id);

        if (this->get_count())
        {
          m_IndexMapping[last_id.id] = idx;
        }
      }
    }
  }

  u64 get_count() const override
  {
    return m_Components.size();
  }

private:
  void destroy_entity_idx(u32 idx)
  {
    const auto size = this->get_count();
    NC_ASSERT(idx < size);

    std::swap(m_Components[idx], m_Components[size-1]);
    m_Components.pop_back();
  }

  template<typename L>
  void for_each(u64 idx, L lambda)
  {
    (this->for_one<CompTypes>(idx, lambda), ...);
  }

  template<typename T, typename L>
  void for_one(u64 idx, L lambda)
  {
    lambda(std::get<T>(m_Components[idx]));
  }

private:
  template<typename T>
  using column = std::vector<T>;

  column<std::tuple<CompTypes...>> m_Components;
  std::unordered_map<u32, u32>     m_IndexMapping;
  u32                              m_LastId = 0;
};

}

