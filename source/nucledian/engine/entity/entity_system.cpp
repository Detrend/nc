// Project Nucledian Source File

#include <common.h>

#include <engine/entity/entity_system.h>

#include <engine/entity/entity_type_definitions.h>
#include <engine/entity/entity.h>
#include <engine/entity/entity_system_listener.h>

namespace nc
{

//==============================================================================
EntityRegistry::EntityRegistry()
{
  static_assert(MAX_POOL_CNT >= EntityTypes::count);
}

//==============================================================================
EntityRegistry::~EntityRegistry() = default;

//==============================================================================
void EntityRegistry::destroy_entity(EntityID id)
{
  nc_assert(id.type < EntityTypes::count);

  auto beg = m_pending_for_destruction.begin();
  auto end = m_pending_for_destruction.end();

  if (std::find(beg, end, id) == end)
  {
    m_pending_for_destruction.push_back(id);

    for (IEntityListener* listener : m_listeners)
    {
      listener->on_entity_garbaged(id);
    }
  }
}

//==============================================================================
void EntityRegistry::cleanup()
{
  for (auto to_destroy : m_pending_for_destruction)
  {
    this->destroy_entity_internal(to_destroy);
  }
}

//==============================================================================
void EntityRegistry::add_listener(IEntityListener* listener)
{
  nc_assert(std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end());
  m_listeners.push_back(listener);
}

//==============================================================================
void EntityRegistry::remove_listener(IEntityListener* listener)
{
  u64 num_erased = std::erase(m_listeners, listener);
  nc_assert(num_erased == 1);
}

//==============================================================================
void EntityRegistry::on_entity_move_internal(EntityID id, vec3 pos, f32 r, f32 h)
{
  for (IEntityListener* listener : m_listeners)
  {
    listener->on_entity_move(id, pos, r, h);
  }
}

//==============================================================================
void EntityRegistry::setup_entity(Entity& entity, EntityID id)
{
  entity.m_id_and_type = id;
  entity.m_registry    = this;

  for (IEntityListener* listener : m_listeners)
  {
    listener->on_entity_create
    (
      id, entity.get_position(), entity.get_radius(), entity.get_height()
    );
  }
}

//==============================================================================
void EntityRegistry::destroy_entity_internal(EntityID id)
{
  nc_assert(id.type < EntityTypes::count);

  Pool& pool = m_pools[id.type];
  auto it = pool.find(id.idx);
  if (it != pool.end())
  {
    for (IEntityListener* listener : m_listeners)
    {
      listener->on_entity_destroy(id);
    }

    // destroys the entity
    pool.erase(it);
  }
}

//==============================================================================
Entity* EntityRegistry::get_entity(EntityID id)
{
  if (id == INVALID_ENTITY_ID)
  {
    return nullptr;
  }

  nc_assert(id.type < EntityTypes::count);

  Pool& pool = m_pools[id.type];
  if (auto it = pool.find(id.idx); it != pool.end())
  {
    return it->second.get();
  }

  return nullptr;
}

//==============================================================================
const Entity* EntityRegistry::get_entity(EntityID id) const
{
  return const_cast<EntityRegistry*>(this)->get_entity(id);
}

}

#ifdef NC_TESTS
#include <engine/entity/entity_tests.inl>
#endif

