// Project Nuclidean Source File
#pragma once

#include <logging.h>
#include <types.h>

#include <engine/autotest/commands.h>
#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/input/game_input.h>

#include <string>

namespace nc
{

struct ModuleEvent;

// Runs one test script, quits the game with exit code 0 when it passes
class AutotestSystem : public IEngineModule
{
public:
  static EngineModuleId get_module_id();

  ~AutotestSystem() override;

  bool init(const std::string& script_path);
  void on_event(ModuleEvent& event) override;

private:
  // Runs the test for one frame
  void update();
  // Runs commands until one needs to wait for the next frame
  void run_commands();

private:
  std::string m_script_path;
  autotest::Script m_script;

  u64 m_command_index = 0;
  u64 m_command_frame = 0;
  PlayerSpecificInputs m_inputs;

  logging::LoggingFunction m_log_listener;
};

}
