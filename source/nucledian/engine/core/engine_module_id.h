// Project Nucledian Source File
#pragma once

#include <types.h>

namespace nc
{
using EngineModuleId = u8;

using EngineModuleMask = u16;
constexpr auto MASK_ALL_MODULES
  = static_cast<EngineModuleMask>(~EngineModuleMask(0));

using ModuleEventId = u8;

}

