// Project Nucledian Source File
#pragma once
#include <types.h>

namespace nc
{

using ComponentID   = u8;
using ComponentMask = u32;
  
struct EntityID
{
  static constexpr ComponentMask COMPONENT_TYPE = 0;
  ComponentMask pool;
  u32           id;
};

template<typename...All>
struct ComponentTypesToFlags;

template<typename First, typename...Other>
struct ComponentTypesToFlags<First, Other...>
{
  static constexpr ComponentMask value = (1 << First::COMPONENT_TYPE) | ComponentTypesToFlags<Other...>::value;
};

template<>
struct ComponentTypesToFlags<>
{
  static constexpr ComponentMask value = 0;
};

}

