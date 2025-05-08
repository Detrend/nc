// Project Nucledian Source File
#pragma once
#include <engine/entity/entity.h>

#include <math/vector.h> // vec3

#include <unordered_map>
#include <array>
#include <memory>

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

  Entity* get_entity(EntityID id);

  void destroy_entity(EntityID id);

  // The signature of the lambda has to be "void(Entity&)"
  template<typename L>
  void for_each(EntityTypeMask types, L lambda);

  template<typename T, typename L>
  void for_each(L lambda);

  const SectorMapping& get_mapping();

private:
  void setup_entity(Entity& entity, EntityID id);

private:
  // Bump this up if the entity count gets larger
  static constexpr u64 MAX_POOL_CNT = 12;

  using Pool  = std::unordered_map<u32, std::unique_ptr<Entity>>;
  using Pools = std::array<Pool, MAX_POOL_CNT>;

  SectorMapping& m_Mapping;
  Pools          m_Pools;
  u32            m_NextID = 0; // incremented on each created entity
};

}

#include <engine/entity/entity_system.inl>