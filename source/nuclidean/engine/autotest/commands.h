// Project Nuclidean Source File
#pragma once

#include <logging.h>
#include <types.h>

#include <engine/input/game_input.h>
#include <engine/player/level_types.h>

#include <game/game_types.h>

#include <array>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace nc::autotest
{

// Tracked game values which can be used in test's conditions
enum class GameValue : u8
{
  player_health,
  player_x,
  player_y,
  player_z,
  player_weapon,
  player_ammo,
  level,
  level_enemies_killed,
  level_enemies,
  level_secrets_found,
  // - //
  count
};

enum class Comparison : u8
{
  equal,
  not_equal,
  less,
  less_equal,
  greater,
  greater_equal,
};

// Compares a game value with a literal
struct Condition
{
  GameValue value = GameValue::player_health;
  Comparison comparison = Comparison::equal;

  using LiteralType = std::variant<f64, LevelName, WeaponType>;
  LiteralType literal{};
};

// Waits for specified number of frames
struct WaitCommand
{
  static constexpr std::string_view NAME = "wait";

  u64 frames = 0;
};

struct PressKeyCommand
{
  static constexpr std::string_view NAME = "press_key";

  PlayerKeyFlags keys = 0;
};

struct ReleaseKeyCommand
{
  static constexpr std::string_view NAME = "release_key";

  PlayerKeyFlags keys = 0;
};

// Holds keys for a specific number of frames, then releases them
struct PressKeyForCommand
{
  static constexpr std::string_view NAME = "press_key_for";

  PlayerKeyFlags keys = 0;
  u64 frames = 1;
};

// Turns the camera, positive yaw turns right and positive pitch looks up
struct TurnCameraCommand
{
  static constexpr std::string_view NAME = "turn_camera";

  f32 yaw_degrees = 0.0f;
  f32 pitch_degrees = 0.0f;
};

// Fails the test if the condition is false
struct ExpectCommand
{
  static constexpr std::string_view NAME = "expect";

  Condition condition;
};

// Waits until the condition is true, fails when the time runs out
struct WaitUntilCommand
{
  static constexpr std::string_view NAME = "wait_until";

  Condition condition;
  u64 timeout_frames = 0;
};

using Command = std::variant
<
  WaitCommand,
  PressKeyCommand,
  ReleaseKeyCommand,
  PressKeyForCommand,
  TurnCameraCommand,
  ExpectCommand,
  WaitUntilCommand
>;

// One command of a script and the line it came from
struct ScriptLine
{
  Command command;
  u32 line_number = 0;
};

constexpr std::array<std::string_view, 4> LOG_LEVEL_NAMES{"none", "log", "warn", "error"};

// Test settings from the @ directives at the top of a script
struct ScriptHeader
{
  LevelName level;
  u64 fps = 60;
  // Test fails when log of this level or higher appears
  logging::LoggingSeverity fail_log_level = logging::LoggingSeverity::error;
};

struct Script
{
  ScriptHeader header;
  std::vector<ScriptLine> lines;
};

enum class Status : u8
{
  running,
  done,
  failed,
};

struct Context
{
  // Frames since the command started
  u64 frame = 0;
  // Player input for the next frame
  PlayerSpecificInputs& inputs;
  std::string& error;
};

// Runs one frame of a command
Status run_command(const Command& command, Context& context);

}
