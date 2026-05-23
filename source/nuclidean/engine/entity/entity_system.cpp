// Project Nuclidean Source File

#include <common.h>

#include <engine/entity/entity_system.h>

#include <engine/entity/entity_type_definitions.h>
#include <engine/entity/entity.h>
#include <engine/entity/entity_system_listener.h>

#include <engine/graphics/entities/sky_box.h>
#include <engine/graphics/entities/lights.h>
#include <engine/graphics/entities/prop.h>
#include <engine/enemies/enemy.h>
#include <engine/player/player.h>
#include <engine/sound/sound_emitter.h>
#include <game/item.h>
#include <game/particle.h>
#include <game/projectile.h>
#include <game/teleport.h>

#include <unordered_map>
#include <algorithm>     // std::swap

namespace nc
{

//==============================================================================
template<typename EntityType>
struct EntityPool : IEntityPool
{
  std::unordered_map<u32, u32> id_to_idx;
  std::vector<EntityType>      entities;
  u32                          next_id = 0; // have to serialize this as well

  virtual Entity* get(EntityID id)     override;
  virtual Entity* get_first()          override;
  virtual void    destroy(EntityID id) override;
  virtual Entity* create(u32& idx_out) override;
  virtual u64     get_cnt()      const override;
  virtual u64     get_stride()   const override;

  virtual u64  size_required()                   const override;
  virtual bool to_bytes(void* to_memory)         const override;
  virtual bool from_bytes(void* from_memory, u64 size) override;
};

//==============================================================================
template<typename EntityType>
Entity* EntityPool<EntityType>::get(EntityID id)
{
  if (auto it = this->id_to_idx.find(id.idx); it != this->id_to_idx.end())
  {
    return &this->entities[it->second];
  }

  return nullptr; // Does not exist
}

//==============================================================================
template<typename EntityType>
Entity* EntityPool<EntityType>::get_first()
{
  if (entities.size())
  {
    return entities.data();
  }

  return nullptr;
}

//==============================================================================
template<typename EntityType>
u64 EntityPool<EntityType>::get_cnt() const
{
  return entities.size();
}

//==============================================================================
template<typename EntityType>
u64 EntityPool<EntityType>::get_stride() const
{
  return sizeof(EntityType);
}

//==============================================================================
template<typename EntityType>
void EntityPool<EntityType>::destroy(EntityID id)
{
  auto it = this->id_to_idx.find(id.idx);
  if (it == this->id_to_idx.end())
  {
    return;
  }

  // Swap with the last one if the size is more than 1
  if (this->entities.size() > 1)
  {
    u32 my_idx  = it->second;
    u32 last_id = this->entities.back().get_id().idx;
    nc_assert(this->id_to_idx.contains(last_id));
    this->id_to_idx[last_id] = my_idx;
    std::swap(this->entities[my_idx], this->entities.back());
  }

  // Pop the last one
  this->entities.pop_back();
  this->id_to_idx.erase(id.idx);
}

//==============================================================================
template<typename EntityType>
Entity* EntityPool<EntityType>::create(u32& idx_out)
{
  u32 id = this->next_id++;
  idx_out = id;
  id_to_idx[id] = cast<u32>(this->entities.size());
  return &this->entities.emplace_back();
}

//==============================================================================
template<typename EntityType>
u64 EntityPool<EntityType>::size_required() const
{
  return this->get_cnt() * this->get_stride() + sizeof(this->next_id);
}

//==============================================================================
template<typename EntityType>
bool EntityPool<EntityType>::to_bytes(void* to_memory) const
{
  u32* ptr_u32 = cast<u32*>(to_memory);

  // Store the cnt at the start
  ptr_u32[0] = this->next_id;

  void* rest = &ptr_u32[1];

  // Store the rest
  std::memcpy(rest, this->entities.data(), sizeof(EntityType) * this->entities.size());

  // All good
  return true;
}

//==============================================================================
template<typename EntityType>
bool EntityPool<EntityType>::from_bytes(void* from_memory, u64 size)
{
  // The count is on the start of the data
  u64 size_without_prefix = size - sizeof(this->next_id);

  if (size_without_prefix % this->get_stride() != 0)
  {
    nc_assert(false);
    return false;
  }

  u32* ptr_u32 = cast<u32*>(from_memory);
  this->next_id = ptr_u32[0];

  void* rest = &ptr_u32[1];

  // Save some space ahead
  this->entities.resize(size_without_prefix / this->get_stride());

  // Copy the rest
  std::memcpy(entities.data(), rest, size);

  return true;
}

//==============================================================================
#define ENTITY_TYPES(xx) \
  xx(SkyBox)             \
  xx(PointLight)         \
  xx(DirectionalLight)   \
  xx(AmbientLight)       \
  xx(Prop)               \
  xx(Enemy)              \
  xx(Player)             \
  xx(SoundEmitter)       \
  xx(Pickup)             \
  xx(Particle)           \
  xx(Projectile)         \
  xx(Teleport)

//==============================================================================
consteval u64 get_entity_type_count()
{
  u32 type_counter = 0;
  #define COUNT_TYPE(_type) type_counter += 1;
  ENTITY_TYPES(COUNT_TYPE);
  return type_counter;
}

//==============================================================================
EntityRegistry::EntityRegistry()
{
  // I know this is stupid, but it was the simples option with the least amount of
  // code required.
  m_pools.resize(EntityTypes::count);

  #define REGISTER_TYPE(_type) \
    m_pools[_type :: get_type_static()] = std::make_unique<EntityPool<_type>>();

  ENTITY_TYPES(REGISTER_TYPE);
  static_assert(get_entity_type_count() == EntityTypes::count,
    "You probably forgot to add the new type into the ENTITY_TYPES x-macro.");
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
}

//==============================================================================
void EntityRegistry::post_init_entity(Entity& entity)
{
  for (IEntityListener* listener : m_listeners)
  {
    listener->on_entity_create
    (
      entity.get_id(),
      entity.get_position(),
      entity.get_radius(),
      entity.get_height()
    );
  }
}

//==============================================================================
void EntityRegistry::destroy_entity_internal(EntityID id)
{
  nc_assert(id.type < EntityTypes::count);

  IEntityPool* pool = m_pools[id.type].get();
  if (pool->get(id))
  {
    for (IEntityListener* listener : m_listeners)
    {
      listener->on_entity_destroy(id);
    }

    // destroys the entity
    pool->destroy(id);
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

  IEntityPool* pool = m_pools[id.type].get();
  return pool->get(id);
}

//==============================================================================
const Entity* EntityRegistry::get_entity(EntityID id) const
{
  return const_cast<EntityRegistry*>(this)->get_entity(id);
}

}

#if NC_TESTS
#include <engine/entity/entity_tests.inl>
#endif

