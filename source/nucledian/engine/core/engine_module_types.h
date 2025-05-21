// Project Nucledian Source File
#pragma once

#include <engine/core/engine_module_id.h>

namespace nc
{

namespace EngineModule
{
  enum value : EngineModuleId
  {
    graphics_system = 0,
    input_system,
    entity_system,
    resource_system,
    sound_system,
    tween_system,
    // -- //
    count
  };
}
static_assert(EngineModule::count <= 16,
  "Number of bits in module bitmask exceeded, "
  "increase the size of engine_module_mask");

}

