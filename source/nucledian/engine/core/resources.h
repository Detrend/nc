#pragma once

#include <types.h>

#include <vector>

namespace nc
{

// Determines how long a resource lives.
enum class ResourceLifetime : u8
{
  None,
  // Resource lives until current level is unloaded. This is typically used for level-specific assets.
  Level,
  // Resources lives until the game is terminated. This is typically used for assets that are shared across levels.
  Game,
};

template <typename TResource>
class ResourceManager;

/**
 * A lightweight, type-safe reference to a resource managed by a `ResourceManager<TResource>`.
 * 
 * A resource is typically created and managed by the `ResourceManager<TResource>`. Direct interaction with the
 * `TResource` class is ussually unnecessary; instead, it is recommended to use `ResourceHandle<TResource>`.
 * 
 * A `ResourceHandle` can become invalid if the resource it references is unloaded.
 */
template<typename TResource>
class ResourceHandle
{
public:
  friend class ResourceManager<TResource>;

  constexpr static ResourceHandle<TResource> invalid();

  /**
   * Checks whether the resource referenced by this handle is still valid.
   * 
   * A resource can become invalid if it is unloaded by the `ResourceManager<TResource>`.
   */
  bool is_valid() const;

private:
  // Constructor is private to ensure that only the `ResourceManager<TResource>` can create handles.
  constexpr ResourceHandle(u32 resource_id, u32 generation, ResourceLifetime lifetime);

  ResourceLifetime m_lifetime = ResourceLifetime::None;
  // Generation of resouce. Used for invalidation of handles when resources are unloaded.
  u32 m_generation = 0;
  /**
   * Resource identifier, used as an index into the internal array of the corresponding ResourceManager<TResource>
   * instance.
   */
  u32 m_resource_id = 0;
};

/**
* Manages the ownership, and lifetime of resources of type `TResource`.
*
* Resources are divided into several groups by their lifetime. For more information see `ResourceLifetime` enum.
*/
template <typename TResource>
class ResourceManager
{
public:
  // Registers a resource with the manager, transferring ownership to it.
  ResourceHandle<TResource> register_resource(TResource&& resource, ResourceLifetime lifetime);
  void unload_resources(ResourceLifetime lifetime);
  const TResource& get_resource(ResourceHandle<TResource> handle) const;

  // Get current resoures generation. Used for invalidation of handles when resources are unloaded.
  static u32 get_generation();

protected:
  ResourceManager() {}

private:
  // Current resoures generation. Used for invalidation of handles when resources are unloaded.
  inline static u32 m_generation = 0;
  std::vector<TResource> m_game_resources;
  std::vector<TResource> m_level_resources;
};

}

#include <engine/core/resources.inl>