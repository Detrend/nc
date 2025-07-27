// Project Nucledian Source File
#pragma once

#include <common.h>

namespace nc
{

//==============================================================================
template<typename Module>
  requires IsEngineModule<Module>
Module& Engine::get_module()
{
  const auto id = Module::get_module_id();

  nc_assert(id < m_modules.size());
  nc_assert(m_modules[id]);
  return *static_cast<Module*>(m_modules[id].get());
}

}
