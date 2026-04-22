#pragma once

#include <engine/core/resources.h>
#include <types.h>

#include <memory>
#include <stdexcept>

namespace nc
{

//==============================================================================
template<typename TResource>
inline constexpr ResourceHandle<TResource> ResourceHandle<TResource>::invalid()
{
  return ResourceHandle<TResource>(0, 0, ResLifetime::None);
}

//==============================================================================
template<typename TResource>
inline bool ResourceHandle<TResource>::is_valid() const
{
  return m_lifetime == ResLifetime::Game || m_generation == ResourceManager<TResource>::get_generation();
}

//==============================================================================
template<typename TResource>
inline constexpr ResourceHandle<TResource>::ResourceHandle(u32 resource_id, u32 generation, ResLifetime lifetime)
  : m_resource_id(resource_id), m_lifetime(lifetime), m_generation(generation) {}

//==============================================================================
template<typename TResource>
inline ResourceHandle<TResource> ResourceManager<TResource>::register_resource
(
  TResource&& resource,
  ResLifetime lifetime
)
{
  switch (lifetime)
  {
    case ResLifetime::Level:
    {
      m_level_resources.push_back(std::forward<TResource>(resource));
      return ResourceHandle<TResource>(static_cast<u32>(m_level_resources.size()) - 1, get_generation(), lifetime);
    }
    case ResLifetime::Game:
    {
      m_game_resources.push_back(std::forward<TResource>(resource));
      return ResourceHandle<TResource>(static_cast<u32>(m_game_resources.size()) - 1, get_generation(), lifetime);
    }
    default:
    {
      throw std::logic_error("Unsupported resource lifetime used for register_resource.");
    }
  }
}

//==============================================================================
template<typename TResource>
inline void ResourceManager<TResource>::unload_resources(ResLifetime lifetime)
{
  switch (lifetime)
  {
  case ResLifetime::Level:
  {
    for (auto& resource : m_level_resources)
    {
      resource.unload();
    }
    m_level_resources.clear();

    m_generation++;
    break;
  }
  case ResLifetime::Game:
  {
    unload_resources(ResLifetime::Level);

    for (auto& resource : m_game_resources)
    {
      resource.unload();
    }
    m_game_resources.clear();
    break;
  }
  default:
  {
    throw std::logic_error("Unsupported resource lifetime used for unload_resources.");
  }
  }
}

//==============================================================================
template<typename TResource>
inline const TResource& ResourceManager<TResource>::get_resource(ResourceHandle<TResource> handle) const
{
  switch (handle.m_lifetime)
  {
    case ResLifetime::Level:
    {
      return m_level_resources[handle.m_resource_id];
    }
    case ResLifetime::Game:
    {
      return m_game_resources[handle.m_resource_id];
    }
    default:
    {
      throw std::logic_error("Unsupported resource lifetime used for get_resource.");
    }
  }
}

//==============================================================================
template<typename TResource>
inline u32 ResourceManager<TResource>::get_generation()
{
  return m_generation;
}

}
