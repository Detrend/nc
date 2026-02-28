// Project Nucledian Source File
#pragma once

#include <types.h>
#include <common.h>
#include <engine/player/level_types.h>
#include <engine/input/game_input.h>

#include <vector>
#include <chrono>
#include <span>

namespace nc
{

using SaveID = u64;

// For old save compatibility with newer game version
using GameVersion = u16;

// Do not change order of items in this enum as it would break backwards
// compatibility.
namespace GameVersions
{
enum evalue : GameVersion
{
  demo_001 = 0, // The initial demo release for software project
};
}

// Change this when the version of the game changes
static constexpr GameVersion CURRENT_GAME_VERSION = GameVersions::demo_001;

// A set of data that persists between levels
NC_PUSH_PACKED
struct LevelPersistentData
{
  // These are just examples how to use it, replace
  // by the actual implementation later
  u16 player_hp = 0;
  u16 ammo      = 0;
};
NC_POP_PACKED

NC_PUSH_PACKED
struct SaveGameData
{
  using Clock    = std::chrono::system_clock;
  using SaveTime = std::chrono::time_point<Clock>;
  static constexpr u64  SIGNATURE_SIZE    = 6;
  static constexpr cstr SIGNATURE         = "ncsave";
  static constexpr u64  LVL_NAME_SIZE     = 32;
  static constexpr u64  CHAPTER_NAME_SIZE = 32;

  char                signature[SIGNATURE_SIZE];
  SaveID              id;
  SaveTime            time;
  char                chapter   [LVL_NAME_SIZE];
  char                level_name[CHAPTER_NAME_SIZE];
  GameVersion         version;
  LevelPersistentData data;
};
NC_POP_PACKED

void serialize_save_game_to_bytes(const SaveGameData& data, std::vector<byte>& bytes_out);
bool deserialize_save_game_from_bytes(SaveGameData& data_out, const std::vector<byte>& bytes);

NC_PUSH_PACKED
struct DemoDataHeader
{
  static constexpr u64  SIGNATURE_SIZE = 6;
  static constexpr cstr SIGNATURE      = "ncdemo";
  static constexpr u64  LVL_NAME_SIZE  = 32;

  char        signature[SIGNATURE_SIZE];
  char        level_name[32]; // Name of the level
  GameVersion version;
  u64         num_frames = 0;
};
NC_POP_PACKED

NC_PUSH_PACKED
struct DemoDataFrame
{
  PlayerSpecificInputs inputs;
  f32                  delta;
};
NC_POP_PACKED

u64 calc_size_for_demo_to_bytes
(
  const DemoDataHeader& header
);

void save_demo_to_bytes
(
  const DemoDataHeader& header,
  const DemoDataFrame*  frames,
  byte*                 bytes_out // Preallocate this yourself
);

bool load_demo_from_bytes
(
  DemoDataHeader&             header_out,
  std::vector<DemoDataFrame>& frames_out,
  const byte*                 bytes_start,
  u64                         bytes_cnt
);

}
