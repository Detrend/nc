// Project Nucledian Source File
#pragma once
#include <engine/entity/entity.h>

#include <math/vector.h> // vec3

#include <unordered_map>
#include <array>
#include <memory>
#include <vector>

// Forward declarations
namespace nc
{
class  Entity;
struct SectorMapping;
struct IEntityListener;
}

namespace nc
{

class EntityRegistry
{
public:
  EntityRegistry();
  ~EntityRegistry();

  EntityRegistry(EntityRegistry&)            = delete;
  EntityRegistry& operator=(EntityRegistry&) = delete;

  template<typename T, typename...CtorArgs>
  T* create_entity(CtorArgs...args);

  template<typename T>
  T* get_entity(EntityID id);

  Entity*       get_entity(EntityID id);
  const Entity* get_entity(EntityID id) const;

  // Schedules entity for destruction. Destroyed on the end
  // of a frame
  void destroy_entity(EntityID id);

  // The signature of the lambda has to be "void(Entity&)"
  template<typename L>
  void for_each(EntityTypeMask types, L lambda);

  template<typename T, typename L>
  void for_each(L lambda);

  // Cleans up entities pending for destruction
  void cleanup();

  // Manipulate the listeners
  void add_listener(IEntityListener* listener);
  void remove_listener(IEntityListener* listener);

  // Called from the entities themselves when moving or rescaling.
  // Do not call on your own or stuff will break.
  void on_entity_move_internal(EntityID id, vec3 pos, f32 r, f32 h);

private:
  void setup_entity(Entity& entity, EntityID id);

  void destroy_entity_internal(EntityID id);

private:
  // Bump this up if the entity count gets larger
  static constexpr u64 MAX_POOL_CNT = 12;

  using Pool      = std::unordered_map<u32, std::unique_ptr<Entity>>;
  using Pools     = std::array<Pool, MAX_POOL_CNT>;
  using IDList    = std::vector<EntityID>;
  using Listeners = std::vector<IEntityListener*>;

  Listeners      m_listeners;
  Pools          m_pools;
  u32            m_next_id = 0; // incremented on each created entity
  IDList         m_pending_for_destruction;
};

}

#include <engine/entity/entity_system.inl>