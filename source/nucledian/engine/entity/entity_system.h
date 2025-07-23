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
}

namespace nc
{

class EntityRegistry
{
public:
  EntityRegistry(SectorMapping& mapping);
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

  const SectorMapping& get_mapping();

  // Cleans up entities pending for destruction
  void cleanup();

private:
  void setup_entity(Entity& entity, EntityID id);

  void destroy_entity_internal(EntityID id);

private:
  // Bump this up if the entity count gets larger
  static constexpr u64 MAX_POOL_CNT = 12;

  using Pool   = std::unordered_map<u32, std::unique_ptr<Entity>>;
  using Pools  = std::array<Pool, MAX_POOL_CNT>;
  using IDList = std::vector<EntityID>;

  SectorMapping& m_Mapping;
  Pools          m_Pools;
  u32            m_NextID = 0; // incremented on each created entity
  IDList         m_pending_for_destruction;
};

}

#include <engine/entity/entity_system.inl>