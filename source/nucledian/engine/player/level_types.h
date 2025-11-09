// Project Nucledian Source File
#pragma once

#include <common.h>
#include <types.h>
#include <metaprogramming.h>

#include <array>
#include <format>
#include <string>

namespace nc
{

constexpr u64 CHAPTER_ID_LENGTH = 24;
constexpr u64 LEVEL_NAME_LENGTH = 24;

// TODO: Replace with static string.
using ChapterID = std::string;
using LevelName = std::string;

inline const LevelName   INVALID_LEVEL_NAME    = std::string{"<invalid_level>"};
inline const ChapterID   INVALID_CHAPTER_ID    = std::string{"<invalid_chapter>"};
inline const std::string LEVELS_DIRECTORY_PATH = std::string{".\\content\\levels"};

inline std::string get_full_level_path(const LevelName& level_name)
{
  return std::format("{0}\\{1}.json", LEVELS_DIRECTORY_PATH, level_name.data());
}

namespace Levels
{
inline const LevelName LEVEL_1 = "level_test1";
inline const LevelName LEVEL_2 = "level_test2";
}

static auto LevelsDB = std::to_array
({
  Levels::LEVEL_1,
  Levels::LEVEL_2
});


}

