// Project Nucledian Source File
#pragma once

#include <types.h>
#include <metaprogramming.h>

#include <array>
#include <format>

namespace nc
{

constexpr u64 CHAPTER_ID_LENGTH = 24;
constexpr u64 LEVEL_NAME_LENGTH = 24;

using ChapterID = std::array<char, CHAPTER_ID_LENGTH>;
using LevelName = std::array<char, LEVEL_NAME_LENGTH>;


#define make_arraystring(length, str) std::to_array((const char[(length)]) (str))


constexpr LevelName INVALID_LEVEL_NAME = make_arraystring(LEVEL_NAME_LENGTH, "<invalid_level>");
constexpr ChapterID INVALID_CHAPTER_ID = make_arraystring(CHAPTER_ID_LENGTH, "<invalid_chapter>");

const std::string LEVELS_DIRECTORY_PATH = ".\\content\\levels";

static inline std::string get_full_level_path(const LevelName& level_name) { return std::format("{0}\\{1}.json", LEVELS_DIRECTORY_PATH, level_name.data()); }

namespace Levels
{
  static constexpr LevelName LEVEL_1 = make_arraystring(LEVEL_NAME_LENGTH, "level_test1");
  static constexpr LevelName LEVEL_2 = make_arraystring(LEVEL_NAME_LENGTH, "level_test2");
}


static auto LevelsDB = std::to_array({
  Levels::LEVEL_1,
  Levels::LEVEL_2
});


}

