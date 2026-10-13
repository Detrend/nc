// Project Nuclidean Source File
#pragma once

#include <engine/render/render_system.h>

#include <memory>

namespace nc
{

//==============================================================================
template<typename TRenderer>
bool RenderSystem::switch_renderer()
{
  // TODO: Load textures used only by the current level into a separate level atlas.
  m_game_atlas.load_directory("content/textures/surface");
  const bool success = set_renderer(std::make_unique<TRenderer>());
  m_game_atlas.free_gpu_data();
  return success;
}

}
