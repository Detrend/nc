// Project Nuclidean Source File
#include <config.h>

#if NC_TESTS

#include <engine/autotest/commands.h>

#include <math/utils.h>
#include <metaprogramming.h>
#include <unit_test.h>

#include <engine/game/game_helpers.h>
#include <engine/game/game_system.h>
#include <engine/player/player.h>

#include <game/weapons.h>

#include <cmath>
#include <format>
#include <numbers>

namespace nc::autotest
{

//==============================================================================
static Status run(const WaitCommand& command, Context& context)
{
  return context.frame < command.frames ? Status::running : Status::done;
}

//==============================================================================
static Status run(const PressKeyCommand& command, Context& context)
{
  context.inputs.keys |= command.keys;
  return Status::done;
}

//==============================================================================
static Status run(const ReleaseKeyCommand& command, Context& context)
{
  context.inputs.keys &= cast<PlayerKeyFlags>(~command.keys);
  return Status::done;
}

//==============================================================================
static Status run(const PressKeyForCommand& command, Context& context)
{
  if (context.frame == 0)
  {
    context.inputs.keys |= command.keys;
  }

  if (context.frame < command.frames)
  {
    return Status::running;
  }

  context.inputs.keys &= cast<PlayerKeyFlags>(~command.keys);
  return Status::done;
}

//==============================================================================
static Status run(const TurnCameraCommand& command, Context& context)
{
  if (context.frame > 0)
    return Status::done;

  context.inputs.analog[PlayerAnalogInputs::look_horizontal] = -deg2rad(command.yaw_degrees);
  context.inputs.analog[PlayerAnalogInputs::look_vertical] = deg2rad(command.pitch_degrees);
  return Status::running;
}

//==============================================================================
// Reads a game value
static Condition::LiteralType read_game_value(GameValue value)
{
  const GameSystem& game_system = GameSystem::get();
  const Player* const player = GameHelpers::get().get_player();

  nc_assert(player, "Player don't exist");

  switch (value)
  {
    case GameValue::player_health: return cast<f64>(player->get_health());
    case GameValue::player_x: return cast<f64>(player->get_position().x);
    case GameValue::player_y: return cast<f64>(player->get_position().y);
    case GameValue::player_z: return cast<f64>(player->get_position().z);
    case GameValue::player_weapon: return player->get_equipped_weapon();
    case GameValue::player_ammo: return cast<f64>(player->get_current_weapon_ammo());
    case GameValue::level: return game_system.get_level_name();
    case GameValue::level_enemies_killed: return cast<f64>(game_system.get_kill_count());
    case GameValue::level_enemies: return cast<f64>(game_system.get_enemy_count());
    case GameValue::level_secrets_found: return cast<f64>(game_system.get_revealed_count());
  }

  nc_assert(false, "Unknown game value");
  return {};
}

//==============================================================================
// Evaluate condition on the game value
static bool compare(const Condition::LiteralType& game_value, const Condition& condition)
{
  switch (condition.comparison)
  {
    case Comparison::equal:         return game_value == condition.literal;
    case Comparison::not_equal:     return game_value != condition.literal;
    case Comparison::less:          return game_value <  condition.literal;
    case Comparison::less_equal:    return game_value <= condition.literal;
    case Comparison::greater:       return game_value >  condition.literal;
    case Comparison::greater_equal: return game_value >= condition.literal;
  }

  nc_assert(false, "Unknown comparison");
  return false;
}

//==============================================================================
static std::string to_string(const Condition::LiteralType& literal)
{
  if (std::holds_alternative<LevelName>(literal))
  {
    return std::get<LevelName>(literal).to_string();
  }

  if (std::holds_alternative<WeaponType>(literal))
  {
    return std::string{meta::enum_value_name(cast<WeaponTypes::evalue>(std::get<WeaponType>(literal)))};
  }

  return std::format("{}", std::get<f64>(literal));
}

//==============================================================================
static Status run(const ExpectCommand& command, Context& context)
{
  const Condition::LiteralType game_value = read_game_value(command.condition.value);
  if (compare(game_value, command.condition))
  {
    return Status::done;
  }

  context.error = std::format("condition is false, value is {}", to_string(game_value));
  return Status::failed;
}

//==============================================================================
static Status run(const WaitUntilCommand& command, Context& context)
{
  const Condition::LiteralType game_value = read_game_value(command.condition.value);
  if (compare(game_value, command.condition))
  {
    return Status::done;
  }

  if (context.frame < command.timeout_frames)
  {
    return Status::running;
  }

  context.error = std::format("timed out, value is {}", to_string(game_value));
  return Status::failed;
}

//==============================================================================
Status run_command(const Command& command, Context& context)
{
  return std::visit([&](const auto& alternative) { return run(alternative, context); }, command);
}

// Unit tests

//==============================================================================
// Runs the command as the given frame since it started
static Status run_test_command(const Command& command, u64 frame, PlayerSpecificInputs& inputs)
{
  std::string error;
  Context context{.frame = frame, .inputs = inputs, .error = error};
  return run_command(command, context);
}

//==============================================================================
static PlayerKeyFlags key_flag(PlayerKeyInputs::values key)
{
  return cast<PlayerKeyFlags>(1 << key);
}

//==============================================================================
static bool autotest_run_wait(unit_test::TestCtx& /*ctx*/)
{
  PlayerSpecificInputs inputs;
  const Command wait = WaitCommand{.frames = 30};

  for (u64 frame = 0; frame < 30; ++frame)
    NC_TEST_ASSERT(run_test_command(wait, frame, inputs) == Status::running);

  NC_TEST_ASSERT(run_test_command(wait, 30, inputs) == Status::done);

  const Command no_wait = WaitCommand{.frames = 0};
  NC_TEST_ASSERT(run_test_command(no_wait, 0, inputs) == Status::done);

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(autotest_run_wait);

//==============================================================================
static bool autotest_run_press_and_release_key(unit_test::TestCtx& /*ctx*/)
{
  PlayerSpecificInputs inputs{.keys = key_flag(PlayerKeyInputs::jump)};

  const Command press = PressKeyCommand{.keys = key_flag(PlayerKeyInputs::forward)};
  NC_TEST_ASSERT(run_test_command(press, 0, inputs) == Status::done);
  NC_TEST_ASSERT(inputs.keys == (key_flag(PlayerKeyInputs::jump) | key_flag(PlayerKeyInputs::forward)));

  // use is not held, releasing it changes nothing
  const PlayerKeyFlags released_keys = key_flag(PlayerKeyInputs::jump) | key_flag(PlayerKeyInputs::use);
  const Command release = ReleaseKeyCommand{.keys = released_keys};
  NC_TEST_ASSERT(run_test_command(release, 0, inputs) == Status::done);
  NC_TEST_ASSERT(inputs.keys == key_flag(PlayerKeyInputs::forward));

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(autotest_run_press_and_release_key);

//==============================================================================
static bool autotest_run_press_key_for(unit_test::TestCtx& /*ctx*/)
{
  PlayerSpecificInputs inputs{.keys = key_flag(PlayerKeyInputs::jump)};
  const Command press_for = PressKeyForCommand{.keys = key_flag(PlayerKeyInputs::forward), .frames = 10};

  for (u64 frame = 0; frame < 10; ++frame)
  {
    NC_TEST_ASSERT(run_test_command(press_for, frame, inputs) == Status::running);
    NC_TEST_ASSERT(inputs.keys == (key_flag(PlayerKeyInputs::jump) | key_flag(PlayerKeyInputs::forward)));
  }

  NC_TEST_ASSERT(run_test_command(press_for, 10, inputs) == Status::done);
  NC_TEST_ASSERT(inputs.keys == key_flag(PlayerKeyInputs::jump));

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(autotest_run_press_key_for);

//==============================================================================
static bool autotest_run_turn_camera(unit_test::TestCtx& /*ctx*/)
{
  constexpr f32 TOLERANCE = 1e-6f;
  constexpr f32 PI = std::numbers::pi_v<f32>;

  PlayerSpecificInputs inputs{.keys = key_flag(PlayerKeyInputs::forward)};
  const Command turn = TurnCameraCommand{.yaw_degrees = 90.0f, .pitch_degrees = 30.0f};

  NC_TEST_ASSERT(run_test_command(turn, 0, inputs) == Status::running);
  NC_TEST_ASSERT(inputs.keys == key_flag(PlayerKeyInputs::forward));

  // moving the mouse right gives a negative horizontal look input, moving it up a positive vertical one
  NC_TEST_ASSERT(std::abs(inputs.analog[PlayerAnalogInputs::look_horizontal] + PI / 2.0f) < TOLERANCE);
  NC_TEST_ASSERT(std::abs(inputs.analog[PlayerAnalogInputs::look_vertical] - PI / 6.0f) < TOLERANCE);

  NC_TEST_ASSERT(run_test_command(turn, 1, inputs) == Status::done);

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(autotest_run_turn_camera);

//==============================================================================
static bool autotest_literal_to_string(unit_test::TestCtx& /*ctx*/)
{
  NC_TEST_ASSERT(to_string(Condition::LiteralType{50.0}) == "50");
  NC_TEST_ASSERT(to_string(Condition::LiteralType{-2.5}) == "-2.5");

  // failure messages print positions that can be pasted back into a script unchanged
  NC_TEST_ASSERT(to_string(Condition::LiteralType{13.399998664855957}) == "13.399998664855957");

  NC_TEST_ASSERT(to_string(Condition::LiteralType{LevelName::Const("level_final1")}) == "level_final1");

  const Condition::LiteralType shotgun{std::in_place_type<WeaponType>, WeaponTypes::shotgun};
  NC_TEST_ASSERT(to_string(shotgun) == "shotgun");

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(autotest_literal_to_string);

}

#endif
