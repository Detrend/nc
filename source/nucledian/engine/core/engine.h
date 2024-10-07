// Project Nucledian Source File
#pragma once

#include <engine/core/is_engine_module.h>

#include <array>
#include <memory>
#include <vector>
#include <string>

namespace nc
{

struct ModuleEvent;
class  IEngineModule;

class Engine
{
public:
  template<typename Module>
    requires IsEngineModule<Module>
  Module& get_module();

  void send_event(ModuleEvent& event);
  void send_event(ModuleEvent&& event);

  bool init();
  void run();
  void terminate();

  // TODO: if we use this in multiple places then
  // probably introduce a quit reason enum as well
  void request_quit();

private:
  bool should_quit();

private:
  using ModuleArray  = std::array<std::unique_ptr<IEngineModule>, 8>;
  using ModuleVector = std::vector<IEngineModule*>;

private:
  ModuleArray  m_modules;
  ModuleVector m_module_init_order;
  bool         m_should_quit = false;
};

Engine& get_engine();

int init_engine_and_run_game(const std::vector<std::string>& args);

}

#include <engine/core/engine.inl>

