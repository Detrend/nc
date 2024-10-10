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
struct EventJournal;

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

  // Installs a new event journal and starts replaying from it
  // from the next frame
  void install_and_replay_event_journal(EventJournal&& journal);
  void stop_event_journal();

  // Might be a nullptr
  void set_recording_journal(EventJournal* journal);

private:
  bool should_quit()             const;
  bool event_journal_installed() const;
  bool event_journal_active()    const;

  // Handles the local state and sends appropriate
  // module messages
  void handle_journal_state_during_update();

private:
  using ModuleArray  = std::array<std::unique_ptr<IEngineModule>, 8>;
  using ModuleVector = std::vector<IEngineModule*>;
  using JournalSmart = std::unique_ptr<EventJournal>;

private:
  ModuleArray   m_modules;
  ModuleVector  m_module_init_order;
  JournalSmart  m_journal;
  EventJournal* m_recorded_journal;
  bool          m_should_quit         : 1 = false;
  bool          m_journal_installed   : 1 = false;
  bool          m_journal_active      : 1 = false;
  bool          m_journal_interrupted : 1 = false;
};

Engine& get_engine();

int init_engine_and_run_game(const std::vector<std::string>& args);

}

#include <engine/core/engine.inl>

