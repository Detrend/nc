// Project Nucledian Source File
#pragma once

#include <types.h>
#include <metaprogramming.h>

#include <array>

namespace nc
{

constexpr u64 CHAPTER_ID_LEN = 16;
using ChapterID = std::array<char, CHAPTER_ID_LEN>;
using LevelID   = u16;

constexpr LevelID   INVALID_LEVEL_ID   = static_cast<LevelID>(-1);
constexpr ChapterID INVALID_CHAPTER_ID = std::to_array("invalid_chapter");

// TODO: temporary solution
namespace Levels
{
  enum evalue : LevelID
  {
    json_map = 0,
    demo_map,
    cool_map,
    portal_test,
    square_map,
    // - //
    count
  };
}

struct LevelData
{
  LevelID next_level = INVALID_LEVEL_ID;
  cstr    name;
};

constexpr cstr LEVEL_NAMES[]
{
  "json map",
  "demo map",
  "cool map",
  "portal test",
  "square map",
};
static_assert(ARRAY_LENGTH(LEVEL_NAMES) == Levels::count);

}

