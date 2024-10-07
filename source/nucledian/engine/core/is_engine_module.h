// Project Nucledian Source File
// Info: Engine module concept. Each new module must satisfy
// the given rules.
#pragma once

#include <engine/core/engine_module_id.h>

#include <concepts>

namespace nc
{

class IEngineModule;
  
// If you are creating a new engine module, then it must be derived
// from IEngineModule class and have a "get_module_id" static method.
// The "get_module_id" must then return a one of the values of the
// engine module types enum (from engine_module_types.h).
template<typename T>
concept IsEngineModule =
std::derived_from<T, IEngineModule> &&
requires
{
  {T::get_module_id()} -> std::same_as<EngineModuleId>;
};

}

