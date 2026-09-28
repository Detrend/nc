// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <types.h>
#include <math/vector.h> // ivec2
#include <common.h>      // cast

#include <bit> // std::bit_cast

template<>
struct std::hash<nc::ivec2>
{
  std::size_t operator()(const nc::ivec2& vec) const noexcept
  {
    struct Shit
    {
      nc::u32 bottom;
      nc::u32 top;
    };

    Shit s;
    s.bottom = cast<nc::u32>(vec.x);
    s.top    = cast<nc::u32>(vec.y);

    return std::bit_cast<std::size_t>(s);
  }
};

namespace nc
{

using EditorID    = u64;
using EditorCoord = ivec2;

template<typename T>
using EID = EditorID;

constexpr EditorID INVALID_EDITOR_ID = 0;
constexpr EditorID VOID_SECTOR_ID    = 1;

}

#endif // #if NC_EDITOR
