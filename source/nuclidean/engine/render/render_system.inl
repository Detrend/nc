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
  return set_renderer(std::make_unique<TRenderer>());
}

}
