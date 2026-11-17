// Project Nuclidean Source File
#include <config.h>

#if NC_TESTS

#include <engine/autotest/autotest_system.h>

#include <engine/autotest/autotest_runner.h>
#include <engine/autotest/parser.h>
#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>
#include <engine/game/game_system.h>
#include <engine/input/input_system.h>

#include <array>
#include <cstdlib>
#include <format>
#include <iostream>
#include <string_view>

namespace nc
{

constexpr std::array LOGGED_SEVERITIES
{
  logging::LoggingSeverity::message,
  logging::LoggingSeverity::warning,
  logging::LoggingSeverity::error,
};

//==============================================================================
// Prints fail reason and exits the game
[[noreturn]] static void fail(std::string_view reason)
{
  std::cout << reason << std::endl;
  std::_Exit(autotest::FAIL_EXIT_CODE);
}

//==============================================================================
EngineModuleId AutotestSystem::get_module_id()
{
  return EngineModule::autotest_system;
}

//==============================================================================
AutotestSystem::~AutotestSystem()
{
  for (const logging::LoggingSeverity severity : LOGGED_SEVERITIES)
    logging::unregister_logging_output(severity, m_log_listener);
}

//==============================================================================
bool AutotestSystem::init(const std::string& script_path)
{
  std::string error;
  if (!autotest::parse_script(script_path, m_script, error))
    fail(error);

  m_script_path = script_path;
  get_engine().set_fixed_frame_time(1.0f / cast<f32>(m_script.header.fps));

  m_log_listener = [this](const std::string& message, const logging::LoggingContext& context)
  {
    const logging::LoggingSeverity fail_level = m_script.header.fail_log_level;
    if (fail_level == logging::LoggingSeverity::unset || context.severity < fail_level)
      return;

    const std::string_view level_name = autotest::LOG_LEVEL_NAMES[cast<u64>(context.severity)];
    fail(std::format("{} reported: \"{}\" in {}, ln.{}", level_name, message, context.file_name, context.line_number));
  };

  for (const logging::LoggingSeverity severity : LOGGED_SEVERITIES)
    logging::register_logging_output(severity, m_log_listener);

  return true;
}

//==============================================================================
void AutotestSystem::on_event(ModuleEvent& event)
{
  switch (event.type)
  {
    case ModuleEventType::post_init:
      GameSystem::get().request_play_level(m_script.header.level);
    break;
    case ModuleEventType::cleanup:
      this->update();
    break;
  }
}

//==============================================================================
void AutotestSystem::update()
{
  // clear camera rotation inputs
  m_inputs = PlayerSpecificInputs{.keys = m_inputs.keys};

  this->run_commands();
  InputSystem::get().set_scripted_inputs(m_inputs);
}

//==============================================================================
void AutotestSystem::run_commands()
{
  std::string error;
  autotest::Context context{.inputs = m_inputs, .error = error};

  while (m_command_index < m_script.lines.size())
  {
    const autotest::ScriptLine& line = m_script.lines[m_command_index];

    context.frame = m_command_frame;
    const autotest::Status status = autotest::run_command(line.command, context);

    if (status == autotest::Status::running)
    {
      ++m_command_frame;
      return;
    }

    if (status == autotest::Status::failed)
      fail(std::format("{}:{} {}", m_script_path, line.line_number, error));

    ++m_command_index;
    m_command_frame = 0;
  }

  get_engine().request_quit();
}

}

#endif
