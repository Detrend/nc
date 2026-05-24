// Project Nuclidean Source File
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
class  Buffer;
}

namespace nc
{

// We can get rid of the virtuals if we ever have problem with them and
// rewrite it with just function pointers... Not worth it now tho.
struct IEntityPool
{
  // Non virtual because called often
  virtual Entity* get(EntityID id)     = 0;
  virtual Entity* get_first()          = 0;
  virtual u64     get_cnt()      const = 0;
  virtual u64     get_stride()   const = 0;
  virtual void    destroy(EntityID id) = 0;
  virtual Entity* create(u32& idx_out) = 0;

  virtual void serialize(Buffer& buffer) = 0;

  virtual ~IEntityPool() = default;
};

class EntityRegistry
{
public:
  EntityRegistry();
  ~EntityRegistry();

  EntityRegistry(EntityRegistry&)            = delete;
  EntityRegistry& operator=(EntityRegistry&) = delete;

  template<typename T, typename...Args>
  T* create_entity(Args&&...args);

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

  // (De)serialization. Performs serialization if "serialize" is true, otherwise
  // does deserialization.
  void serialize(Buffer& buffer);

private:
  void setup_entity(Entity& entity, EntityID id);

  void post_init_entity(Entity& entity);

  void destroy_entity_internal(EntityID id);

private:
  using Pools     = std::vector<std::unique_ptr<IEntityPool>>;
  using IDList    = std::vector<EntityID>;
  using Listeners = std::vector<IEntityListener*>;

  Listeners m_listeners;
  Pools     m_pools;
  IDList    m_pending_for_destruction;
};

}

#include <engine/entity/entity_system.inl>