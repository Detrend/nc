// Project Nucledian Source File
#pragma once

#include <types.h>
#include <metaprogramming.h>

namespace nc
{

using LevelID = u16;
constexpr LevelID INVALID_LEVEL_ID = static_cast<LevelID>(-1);

// TODO: temporary solution
namespace Levels
{
  enum evalue : LevelID
  {
    demo_map = 0,
    cool_map,
    portal_test,
    square_map,
    // - //
    count
  };
}

constexpr cstr LEVEL_NAMES[]
{
  "demo map",
  "cool map",
  "portal test",
  "square map",
};
static_assert(ARRAY_LENGTH(LEVEL_NAMES) == Levels::count);

}

