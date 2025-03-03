#pragma once

#include <engine/graphics/resources/model.h>

namespace nc
{

//==============================================================================
template<ResLifetime lifetime>
inline void ModelManager::unload()
{
  static_assert(lifetime == ResLifetime::Level || lifetime == ResLifetime::Game);

  if constexpr (lifetime == ResLifetime::Level)
  {
    m_level_models.clear();
  }
  else
  {
    m_game_models.clear();
  }
}

}
