// Project Nuclidean Source File
#include <config.h>

#if NC_TESTS

#include <engine/autotest/parser.h>

#include <metaprogramming.h>
#include <stack_vector.h>
#include <unit_test.h>

#include <game/weapons.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <utility>

namespace nc::autotest
{

using Tokens = std::span<const std::string_view>;

constexpr u64 DEFAULT_TIMEOUT_FRAMES = 600;
constexpr std::array<std::string_view, 6> COMPARISON_NAMES{"==", "!=", "<", "<=", ">", ">="};

//==============================================================================
static bool parse_number(std::string_view token, f64& out_number)
{
  const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), out_number);
  return error == std::errc{} && end == token.data() + token.size();
}

//==============================================================================
// Reads frames written as "30f"
static bool parse_duration(std::string_view token, u64& out_frames)
{
  if (!token.ends_with('f'))
    return false;

  const auto [end, error] = std::from_chars(token.data(), token.data() + token.size() - 1, out_frames);
  return error == std::errc{} && end == token.data() + token.size() - 1;
}

//==============================================================================
static bool parse_keys(Tokens tokens, PlayerKeyFlags& out_keys, std::string& out_error)
{
  if (tokens.empty())
  {
    out_error = "expected keys";
    return false;
  }

  for (std::string_view token : tokens)
  {
    PlayerKeyInputs::values key = PlayerKeyInputs::forward;
    if (!meta::parse_enum(token, key))
    {
      out_error = std::format("unknown key \"{}\"", token);
      return false;
    }

    out_keys |= 1 << key;
  }

  return true;
}

//==============================================================================
static bool level_exists(std::string_view name)
{
  return std::filesystem::exists(LEVELS_DIRECTORY_PATH / std::format("{}.json", name));
}

//==============================================================================
// Reads "<game value> <comparison> <literal>"
static bool parse_condition(Tokens tokens, Condition& out_condition, std::string& out_error)
{
  if (tokens.size() != 3)
  {
    out_error = "expected <game value> <comparison> <literal>";
    return false;
  }

  const auto comparison = std::find(COMPARISON_NAMES.begin(), COMPARISON_NAMES.end(), tokens[1]);
  if (!meta::parse_enum(tokens[0], out_condition.value) || comparison == COMPARISON_NAMES.end())
  {
    out_error = std::format("unknown game value \"{}\" or comparison \"{}\"", tokens[0], tokens[1]);
    return false;
  }

  out_condition.comparison = cast<Comparison>(comparison - COMPARISON_NAMES.begin());

  if (out_condition.value == GameValue::level)
  {
    if (!level_exists(tokens[2]))
    {
      out_error = std::format("unknown level \"{}\"", tokens[2]);
      return false;
    }

    out_condition.literal = LevelName{tokens[2]};
    return true;
  }

  if (out_condition.value == GameValue::player_weapon)
  {
    WeaponTypes::evalue weapon = WeaponTypes::wrench;
    if (!meta::parse_enum(tokens[2], weapon))
    {
      out_error = std::format("unknown weapon \"{}\"", tokens[2]);
      return false;
    }

    out_condition.literal = cast<WeaponType>(weapon);
    return true;
  }

  f64 number = 0.0;
  if (!parse_number(tokens[2], number))
  {
    out_error = std::format("expected a number, got \"{}\"", tokens[2]);
    return false;
  }

  out_condition.literal = number;
  return true;
}

//==============================================================================
static bool parse_arguments(Tokens arguments, WaitCommand& out_command, std::string& out_error)
{
  if (arguments.size() == 1 && parse_duration(arguments[0], out_command.frames))
    return true;

  out_error = "expected a duration like 30f";
  return false;
}

//==============================================================================
static bool parse_arguments(Tokens arguments, PressKeyCommand& out_command, std::string& out_error)
{
  return parse_keys(arguments, out_command.keys, out_error);
}

//==============================================================================
static bool parse_arguments(Tokens arguments, ReleaseKeyCommand& out_command, std::string& out_error)
{
  return parse_keys(arguments, out_command.keys, out_error);
}

//==============================================================================
static bool parse_arguments(Tokens arguments, PressKeyForCommand& out_command, std::string& out_error)
{
  if (arguments.size() < 2 || !parse_duration(arguments.back(), out_command.frames))
  {
    out_error = "expected keys followed by a duration like 30f";
    return false;
  }

  return parse_keys(arguments.first(arguments.size() - 1), out_command.keys, out_error);
}

//==============================================================================
static bool parse_arguments(Tokens arguments, TurnCameraCommand& out_command, std::string& out_error)
{
  f64 yaw = 0.0;
  f64 pitch = 0.0;

  const bool valid_count = arguments.size() == 1 || arguments.size() == 2;
  const bool has_pitch = arguments.size() == 2;
  if (!valid_count || !parse_number(arguments[0], yaw) || (has_pitch && !parse_number(arguments[1], pitch)))
  {
    out_error = "expected yaw and an optional pitch in degrees";
    return false;
  }

  out_command.yaw_degrees = cast<f32>(yaw);
  out_command.pitch_degrees = cast<f32>(pitch);
  return true;
}

//==============================================================================
static bool parse_arguments(Tokens arguments, ExpectCommand& out_command, std::string& out_error)
{
  return parse_condition(arguments, out_command.condition, out_error);
}

//==============================================================================
static bool parse_arguments(Tokens arguments, WaitUntilCommand& out_command, std::string& out_error)
{
  out_command.timeout_frames = DEFAULT_TIMEOUT_FRAMES;

  const bool has_timeout = arguments.size() == 4;
  if (has_timeout && !parse_duration(arguments[3], out_command.timeout_frames))
  {
    out_error = "expected a timeout like 30f after the condition";
    return false;
  }

  return parse_condition(has_timeout ? arguments.first(3) : arguments, out_command.condition, out_error);
}

//==============================================================================
// Reads one "@key value" line of the script header
static bool parse_directive(Tokens tokens, ScriptHeader& out_header, std::string& out_error)
{
  if (tokens.size() == 2 && tokens[0] == "@level" && level_exists(tokens[1]))
  {
    out_header.level = LevelName{tokens[1]};
    return true;
  }

  f64 fps = 0.0;
  if (tokens.size() == 2 && tokens[0] == "@fps" && parse_number(tokens[1], fps) && fps >= 1.0)
  {
    out_header.fps = cast<u64>(fps);
    return true;
  }

  const auto log_level = std::find(LOG_LEVEL_NAMES.begin(), LOG_LEVEL_NAMES.end(), tokens.back());
  if (tokens.size() == 2 && tokens[0] == "@fail_log_level" && log_level != LOG_LEVEL_NAMES.end())
  {
    out_header.fail_log_level = cast<logging::LoggingSeverity>(log_level - LOG_LEVEL_NAMES.begin());
    return true;
  }

  out_error = "expected @level <existing level>, @fps <frames per second> or @fail_log_level none|log|warn|error";
  return false;
}

//==============================================================================
template<u64 Index = 0>
static bool parse_command(Tokens tokens, Command& out_command, std::string& out_error)
{
  if constexpr (Index == std::variant_size_v<Command>)
  {
    out_error = std::format("unknown command \"{}\"", tokens[0]);
    return false;
  }
  else
  {
    using CommandType = std::variant_alternative_t<Index, Command>;
    if (tokens[0] != CommandType::NAME)
      return parse_command<Index + 1>(tokens, out_command, out_error);

    CommandType command;
    if (!parse_arguments(tokens.subspan(1), command, out_error))
      return false;

    out_command = command;
    return true;
  }
}

//==============================================================================
bool parse_script(const std::string& path, Script& out_script, std::string& out_error)
{
  std::ifstream file{path};
  if (!file)
  {
    out_error = std::format("{} can't be opened", path);
    return false;
  }

  const std::string source{std::istreambuf_iterator<char>{file}, {}};

  u32 line_number = 0;
  for (const auto range : std::views::split(std::string_view{source}, '\n'))
  {
    ++line_number;
    std::string_view line{range.begin(), range.end()};

    if (line.starts_with("#"))
      continue;

    StackVector<std::string_view, 4> tokens;
    for (const auto token : std::views::split(line, ' '))
    {
      if (token.empty())
        continue;
      tokens.emplace_back(token.begin(), token.end());
    }

    if (tokens.empty())
      continue;

    ScriptLine script_line{.line_number = line_number};
    const bool is_directive = tokens[0].starts_with('@');
    const bool is_parsed = is_directive ? parse_directive(tokens, out_script.header, out_error)
                                        : parse_command(tokens, script_line.command, out_error);
    if (!is_parsed)
    {
      out_error = std::format("{}:{} {}", path, line_number, out_error);
      return false;
    }

    if (!is_directive)
      out_script.lines.push_back(script_line);
  }

  if (out_script.header.level == LevelName{})
  {
    out_error = std::format("{} has no @level", path);
    return false;
  }

  return true;
}

// Unit tests

constexpr std::string_view TEST_HEADER = "@level level_final1\n";

//==============================================================================
static std::string get_test_script_path()
{
  return (std::filesystem::temp_directory_path() / "nc_autotest_parser_test.nct").string();
}

//==============================================================================
// Writes the text to a temporary script file and parses it
static bool parse_test_script(std::string_view text, Script& out_script, std::string& out_error)
{
  const std::string path = get_test_script_path();
  {
    std::ofstream file{path, std::ios::binary};
    file << text;
  }

  const bool parsed = parse_script(path, out_script, out_error);
  std::filesystem::remove(path);
  return parsed;
}

//==============================================================================
// Parses a valid header followed by one command line, empty when it fails or the command has another type
template<typename CommandType>
static std::optional<CommandType> parse_test_command(std::string_view line)
{
  Script script;
  std::string error;
  if (!parse_test_script(std::format("{}{}\n", TEST_HEADER, line), script, error) || script.lines.size() != 1)
    return std::nullopt;

  const CommandType* const command = std::get_if<CommandType>(&script.lines.front().command);
  if (!command)
    return std::nullopt;

  return *command;
}

//==============================================================================
static PlayerKeyFlags key_flag(PlayerKeyInputs::values key)
{
  return cast<PlayerKeyFlags>(1 << key);
}

//==============================================================================
static bool autotest_parse_script_header(unit_test::TestCtx& /*ctx*/)
{
  std::string error;

  Script defaults;
  NC_TEST_ASSERT(parse_test_script(TEST_HEADER, defaults, error));
  NC_TEST_ASSERT(defaults.header.level == LevelName::Const("level_final1"));
  NC_TEST_ASSERT(defaults.header.fps == 60);
  NC_TEST_ASSERT(defaults.header.fail_log_level == logging::LoggingSeverity::error);
  NC_TEST_ASSERT(defaults.lines.empty());

  Script script;
  NC_TEST_ASSERT(parse_test_script("@fps 30\n@level level_final2\n", script, error));
  NC_TEST_ASSERT(script.header.level == LevelName::Const("level_final2"));
  NC_TEST_ASSERT(script.header.fps == 30);

  constexpr std::array<std::pair<std::string_view, logging::LoggingSeverity>, 4> FAIL_LOG_LEVELS
  {{
    {"none",  logging::LoggingSeverity::unset},
    {"log",   logging::LoggingSeverity::message},
    {"warn",  logging::LoggingSeverity::warning},
    {"error", logging::LoggingSeverity::error},
  }};

  for (const auto& [name, severity] : FAIL_LOG_LEVELS)
  {
    Script log_script;
    NC_TEST_ASSERT(parse_test_script(std::format("{}@fail_log_level {}\n", TEST_HEADER, name), log_script, error));
    NC_TEST_ASSERT(log_script.header.fail_log_level == severity);
  }

  constexpr std::array<std::string_view, 9> INVALID_HEADERS
  {
    "",
    "wait 1f\n",
    "@level\n",
    "@level no_such_level\n",
    "@level level_final1 level_final2\n",
    "@level level_final1\n@fps 0\n",
    "@level level_final1\n@fps fast\n",
    "@level level_final1\n@fail_log_level verbose\n",
    "@level level_final1\n@speed 2\n",
  };

  for (std::string_view header : INVALID_HEADERS)
  {
    Script invalid_script;
    NC_TEST_ASSERT(!parse_test_script(header, invalid_script, error));
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(autotest_parse_script_header);

//==============================================================================
static bool autotest_parse_script_commands(unit_test::TestCtx& /*ctx*/)
{
  const auto wait = parse_test_command<WaitCommand>("wait 30f");
  NC_TEST_ASSERT(wait && wait->frames == 30);

  const auto wait_zero = parse_test_command<WaitCommand>("wait 0f");
  NC_TEST_ASSERT(wait_zero && wait_zero->frames == 0);

  const auto press = parse_test_command<PressKeyCommand>("press_key forward backward left right jump");
  NC_TEST_ASSERT(press);
  NC_TEST_ASSERT
  (
    press->keys == (key_flag(PlayerKeyInputs::forward) | key_flag(PlayerKeyInputs::backward)
                  | key_flag(PlayerKeyInputs::left) | key_flag(PlayerKeyInputs::right)
                  | key_flag(PlayerKeyInputs::jump))
  );

  const auto release = parse_test_command<ReleaseKeyCommand>("release_key weapon_3");
  NC_TEST_ASSERT(release && release->keys == key_flag(PlayerKeyInputs::weapon_3));

  const auto press_for = parse_test_command<PressKeyForCommand>("press_key_for left right 15f");
  NC_TEST_ASSERT(press_for && press_for->frames == 15);
  NC_TEST_ASSERT(press_for->keys == (key_flag(PlayerKeyInputs::left) | key_flag(PlayerKeyInputs::right)));

  const auto turn_yaw = parse_test_command<TurnCameraCommand>("turn_camera 90");
  NC_TEST_ASSERT(turn_yaw && turn_yaw->yaw_degrees == 90.0f && turn_yaw->pitch_degrees == 0.0f);

  const auto turn_both = parse_test_command<TurnCameraCommand>("turn_camera -45 10.5");
  NC_TEST_ASSERT(turn_both && turn_both->yaw_degrees == -45.0f && turn_both->pitch_degrees == 10.5f);

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(autotest_parse_script_commands);

//==============================================================================
static bool autotest_parse_script_conditions(unit_test::TestCtx& /*ctx*/)
{
  const auto health = parse_test_command<ExpectCommand>("expect player_health > 0");
  NC_TEST_ASSERT(health);
  NC_TEST_ASSERT(health->condition.value == GameValue::player_health);
  NC_TEST_ASSERT(health->condition.comparison == Comparison::greater);
  NC_TEST_ASSERT(health->condition.literal == Condition::LiteralType{0.0});

  const auto weapon = parse_test_command<ExpectCommand>("expect player_weapon == shotgun");
  const Condition::LiteralType shotgun{std::in_place_type<WeaponType>, WeaponTypes::shotgun};
  NC_TEST_ASSERT(weapon && weapon->condition.value == GameValue::player_weapon);
  NC_TEST_ASSERT(weapon->condition.literal == shotgun);

  const auto level = parse_test_command<ExpectCommand>("expect level != level_final2");
  NC_TEST_ASSERT(level && level->condition.value == GameValue::level);
  NC_TEST_ASSERT(level->condition.comparison == Comparison::not_equal);
  NC_TEST_ASSERT(level->condition.literal == Condition::LiteralType{LevelName::Const("level_final2")});

  const auto wait_with_timeout = parse_test_command<WaitUntilCommand>("wait_until player_x <= -2.5 120f");
  NC_TEST_ASSERT(wait_with_timeout && wait_with_timeout->timeout_frames == 120);
  NC_TEST_ASSERT(wait_with_timeout->condition.value == GameValue::player_x);
  NC_TEST_ASSERT(wait_with_timeout->condition.comparison == Comparison::less_equal);
  NC_TEST_ASSERT(wait_with_timeout->condition.literal == Condition::LiteralType{-2.5});

  // a missing timeout falls back to a default, not to zero frames
  const auto wait_default_timeout = parse_test_command<WaitUntilCommand>("wait_until level_enemies_killed == 3");
  NC_TEST_ASSERT(wait_default_timeout && wait_default_timeout->timeout_frames > 0);
  NC_TEST_ASSERT(wait_default_timeout->condition.value == GameValue::level_enemies_killed);
  NC_TEST_ASSERT(wait_default_timeout->condition.literal == Condition::LiteralType{3.0});

  constexpr std::array<std::pair<std::string_view, Comparison>, 6> COMPARISONS
  {{
    {"==", Comparison::equal},
    {"!=", Comparison::not_equal},
    {"<",  Comparison::less},
    {"<=", Comparison::less_equal},
    {">",  Comparison::greater},
    {">=", Comparison::greater_equal},
  }};

  for (const auto& [name, comparison] : COMPARISONS)
  {
    const auto expect = parse_test_command<ExpectCommand>(std::format("expect level_secrets_found {} 1", name));
    NC_TEST_ASSERT(expect && expect->condition.comparison == comparison);
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(autotest_parse_script_conditions);

//==============================================================================
static bool autotest_parse_script_errors(unit_test::TestCtx& /*ctx*/)
{
  constexpr std::array<std::string_view, 24> INVALID_COMMANDS
  {
    "wait",
    "wait 30",
    "wait f",
    "wait -5f",
    "wait 30f 10f",
    "press_key",
    "press_key fly",
    "press_key count",
    "press_key_for forward",
    "press_key_for 30f",
    "press_key_for fly 30f",
    "release_key",
    "turn_camera",
    "turn_camera left",
    "turn_camera 1 2 3",
    "expect player_health",
    "expect player_health > 0 1",
    "expect player_health => 0",
    "expect player_mana > 0",
    "expect player_health > lots",
    "expect level == no_such_level",
    "expect player_weapon == bfg",
    "wait_until player_health > 0 soon",
    "teleport 10f",
  };

  // the command is on the second line of the script
  const std::string error_prefix = std::format("{}:2 ", get_test_script_path());

  for (std::string_view line : INVALID_COMMANDS)
  {
    Script script;
    std::string error;
    NC_TEST_ASSERT(!parse_test_script(std::format("{}{}\n", TEST_HEADER, line), script, error));
    NC_TEST_ASSERT(error.starts_with(error_prefix));
  }

  Script script;
  std::string error;
  const std::string missing_path = (std::filesystem::temp_directory_path() / "nc_autotest_missing.nct").string();
  NC_TEST_ASSERT(!parse_script(missing_path, script, error));

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(autotest_parse_script_errors);

//==============================================================================
static bool autotest_parse_script_layout(unit_test::TestCtx& /*ctx*/)
{
  Script script;
  std::string error;
  const bool parsed = parse_test_script
  (
    "# comment before the header\n"
    "@level level_final1\n"
    "\n"
    "  wait   1f  \n"
    "# wait 2f\n"
    "wait 3f",
    script, error
  );

  NC_TEST_ASSERT(parsed && script.lines.size() == 2);

  const WaitCommand* const first = std::get_if<WaitCommand>(&script.lines[0].command);
  NC_TEST_ASSERT(first && first->frames == 1 && script.lines[0].line_number == 4);

  const WaitCommand* const last = std::get_if<WaitCommand>(&script.lines[1].command);
  NC_TEST_ASSERT(last && last->frames == 3 && script.lines[1].line_number == 6);

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(autotest_parse_script_layout);

//==============================================================================
// Scripts checked out with Windows line endings
static bool autotest_parse_script_crlf(unit_test::TestCtx& /*ctx*/)
{
  Script script;
  std::string error;
  NC_TEST_ASSERT(parse_test_script("@level level_final1\r\n# comment\r\nwait 30f\r\n", script, error));
  NC_TEST_ASSERT(script.lines.size() == 1 && script.lines[0].line_number == 3);

  const WaitCommand* const wait = std::get_if<WaitCommand>(&script.lines[0].command);
  NC_TEST_ASSERT(wait && wait->frames == 30);

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(autotest_parse_script_crlf);

}

#endif
