// Project Nucledian Source File

#pragma once

namespace nc
{

//==============================================================================
template<typename Module>
  requires IsEngineModule<Module>
Module& Engine::get_module()
{
  const auto id = Module::get_module_id();

  NC_ASSERT(id < m_modules.size());
  NC_ASSERT(m_modules[id]);
  return *static_cast<Module*>(m_modules[id].get());
}

}

