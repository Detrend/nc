// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <common.h>
#include <engine/player/level_types.h>
#include <engine/input/game_input.h>
#include <token.h>

#include <vector>
#include <chrono>
#include <span>

namespace nc
{

using SaveID = u64;

// For old save compatibility with newer game version
using GameVersion = u64;

// Do not change order of items in this enum as it would break backwards
// compatibility.
namespace GameVersions
{
enum evalue : GameVersion
{
  demo_001 = 0, // The initial demo release for software project
};
}

// ===========================================================================//
// ! Change this when the version of the game changes !                       //
constexpr GameVersion CURRENT_GAME_VERSION = GameVersions::demo_001;          //
// ===========================================================================//

constexpr cstr SAVE_DIR_RELATIVE = "save";
constexpr cstr DEMO_DIR_RELATIVE = "demo";
constexpr cstr SAVE_FILE_SUFFIX  = ".ncs";
constexpr cstr DEMO_FILE_SUFFIX  = ".ncd";

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

//NC_PUSH_PACKED
struct SaveGameHeader
{
  using Clock    = std::chrono::system_clock;
  using SaveTime = std::chrono::time_point<Clock>;
  static constexpr u64  SIGNATURE_SIZE    = 6;
  static constexpr cstr SIGNATURE         = "ncsave";

  // These 3 are the only mandatory members of the header. These 2 will ALWAYS
  // be present no matter the game version.
  // The remaining members can change between updates.
  char        signature[SIGNATURE_SIZE];
  GameVersion version;
  SaveTime    time;

  // These can change between versions
  Token chapter;
  Token level;
};
//NC_POP_PACKED

struct LevelTransitionData
{
  s32 health         = 100;
  u32 owned_weapons  = 0;
  u8  current_weapon = 0;
  s32 ammo[4] = {-1, 0, 0, 0};

  bool is_empty() const
  {
    return health >= 0;
  }

  void set_empty()
  {
    health = -67;
  }
};

NC_PUSH_PACKED
struct DemoDataHeader
{
  static constexpr u64  SIGNATURE_SIZE = 6;
  static constexpr cstr SIGNATURE      = "ncdemo";

  char                signature[SIGNATURE_SIZE];
  Token               level_name;
  GameVersion         version;
  u64                 num_frames = 0;
  LevelTransitionData transition_data;
};
NC_POP_PACKED

NC_PUSH_PACKED
struct DemoDataFrame
{
  PlayerSpecificInputs inputs;
  f32                  delta;
};
NC_POP_PACKED

using DemoDataFrames = std::vector<DemoDataFrame>;

// Loads a given demo from a file
bool load_demo_from_file
(
  const std::string&   file,
  LevelName&           level_name_out,
  LevelTransitionData& transition_out,
  DemoDataFrames&      frames_out
);

// Returns a list of available demos
std::vector<std::string> list_available_demo_files();

// Returns a list of available save files
std::vector<std::string> list_available_save_files();

u64 calc_size_for_demo_to_bytes
(
  const DemoDataHeader& header
);

void save_bytes_to_file
(
  const std::string& path,
  void*              data,
  u64                size
);

void* load_bytes_from_file
(
  const std::string& path,
  u64&               size
);

void save_demo_to_bytes
(
  const DemoDataHeader& header,
  const DemoDataFrame*  frames,
  byte*                 bytes_out // Preallocate this yourself
);

bool load_demo_from_bytes
(
  DemoDataHeader& header_out,
  DemoDataFrames& frames_out,
  const byte*     bytes_start,
  u64             bytes_cnt
);

void save_demo_to_file
(
  const std::string&   filename, // No path or extension, only filename
  LevelName            level_name, // Name of the level
  const DemoDataFrame* frames,
  u64                  frames_cnt
);

}
