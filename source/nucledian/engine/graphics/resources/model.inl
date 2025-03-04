#pragma once

#include <engine/graphics/resources/model.h>

namespace nc
{
 //==============================================================================
template<ResLifetime lifetime>
inline ModelHandle ModelManager::add(const Mesh& mesh, const Material& material)
{
  auto& storage = get_storage<lifetime>();
  const u32 id = static_cast<u32>(storage.size());

  storage.emplace_back(mesh, material);
  return ModelHandle(id, lifetime);
}

//==============================================================================
template<ResLifetime lifetime>
inline void ModelManager::unload()
{
  auto& storage = get_storage<lifetime>();
  storage.clear();
}

//==============================================================================
template<ResLifetime lifetime>
inline std::vector<Model>& ModelManager::get_storage()
{
  static_assert(lifetime == ResLifetime::Level || lifetime == ResLifetime::Game);

  if constexpr (lifetime == ResLifetime::Level)
  {
    return m_level_models;
  }
  else
  {
    return m_game_models;
  }
}

}
