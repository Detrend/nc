// Project Nucledian Source File
#pragma once

#include <types.h>
#include <engine/player/level_types.h>

#include <vector>
#include <chrono>

namespace nc
{

using SaveID = u64;

// For old save compatibility with newer game version
using GameVersion = u16;
namespace GameVersions
{
enum evalue : GameVersion
{
  v0 = 0,
  v1,
  v2,
  v3,
};
}

// A set of data that persists between levels
struct LevelPersistentData
{
  // These are just examples how to use it, replace
  // by the actual implementation later
  u16 player_hp = 0;
  u16 ammo      = 0;
};

struct SaveGameData
{
  using Clock    = std::chrono::system_clock;
  using SaveTime = std::chrono::time_point<Clock>;

  SaveID              id;
  SaveTime            time;
  ChapterID           chapter    = INVALID_CHAPTER_ID;
  LevelName           last_level = INVALID_LEVEL_NAME;
  GameVersion         version;
  LevelPersistentData data;
};

void serialize_save_game_to_bytes(const SaveGameData& data, std::vector<byte>& bytes_out);
bool deserialize_save_game_from_bytes(SaveGameData& data_out, const std::vector<byte>& bytes);

}