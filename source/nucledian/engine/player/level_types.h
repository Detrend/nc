// Project Nucledian Source File
#pragma once

#include <common.h>
#include <types.h>
#include <metaprogramming.h>

#include <array>
#include <format>
#include <string>

#include<token.h>

namespace nc
{

constexpr u64 CHAPTER_ID_LENGTH = 24;
constexpr u64 LEVEL_NAME_LENGTH = 24;

// TODO: Replace with static string.
using ChapterID = Token;
using LevelName = Token;

inline const LevelName   INVALID_LEVEL_NAME    {"_no_lvl"};
inline const ChapterID   INVALID_CHAPTER_ID    {"_no_chapter"};
inline const std::string LEVELS_DIRECTORY_PATH {".\\content\\levels"};

inline std::string get_full_level_path(const LevelName& level_name)
{
  return std::format("{0}\\{1}.json", LEVELS_DIRECTORY_PATH, level_name.to_cstring().data());
}

namespace Levels
{
inline constexpr LevelName TEST_LEVEL("level_test1");
inline constexpr LevelName LEVEL_1("level_final1");
inline constexpr LevelName LEVEL_2("level_final2");
inline constexpr LevelName LEVEL_3("level_final3");
}

static auto LevelsDB = std::to_array
({
  Levels::TEST_LEVEL,
  Levels::LEVEL_1,
  Levels::LEVEL_2,
  Levels::LEVEL_3,
});


}

