// Project Nucledian Source File
#include <common.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module.h>
#include <engine/core/module_event.h>
#include <engine/core/is_engine_module.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/event_journal.h>

#include <engine/graphics/graphics_system.h>
#include <engine/input/input_system.h>

#ifdef NC_PROFILING
#include <benchmark/benchmark.h>
#include <algorithm>
#endif

#include <ranges>
#include <chrono>

namespace nc
{

static Engine* g_engine = nullptr;

//==============================================================================
namespace engine_utils
{

//==============================================================================
static f32 duration_to_seconds(auto t1, auto t2)
{
  using namespace std::chrono;
  NC_ASSERT(t2 >= t1);

  return duration_cast<microseconds>(t2 - t1).count() / 1'000'000.0f;
}

}

//==============================================================================
Engine& get_engine()
{
  NC_ASSERT(g_engine);
  return *g_engine;
}

//==============================================================================
#ifdef NC_BENCHMARK
static bool execute_benchmarks_if_required(const std::vector<std::string>& args)
{
  constexpr cstr BENCHMARK_ARG = "-benchmark";

  auto benchmark_arg_it = std::find_if(
    args.begin(),
    args.end(),
    [&](const std::string& str)
    {
      return str == BENCHMARK_ARG;
    });

  if (benchmark_arg_it == args.end())
  {
    // do not run benchmarks
    return false;
  }

  // transform the array of remaining arguments from std::strings to an
  // array of c-strings
  std::vector<char*> c_str_args;
  std::transform(
    benchmark_arg_it+1,
    args.end(),
    std::back_inserter(c_str_args),
    [&](const std::string& string)
    {
      return const_cast<char*>(string.c_str());
    });

  int    argc = static_cast<int>(c_str_args.size());
  char** argv = c_str_args.data();

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
  {
    // TODO: log out failure
    return true;
  }

  ::benchmark::RunSpecifiedBenchmarks();
  ::benchmark::Shutdown();
  return true;
}

#endif

//==============================================================================
int init_engine_and_run_game([[maybe_unused]]const std::vector<std::string>& args)
{
  #ifdef NC_BENCHMARK
  if (execute_benchmarks_if_required(args))
  {
    // only benchmark run, exit
    return 0;
  }
  #endif

  // create instance of the engine
  g_engine = new Engine();

  if (!g_engine->init())
  {
    // failed to init, end it here
    // do something here
    delete g_engine;
    return 1;
  }

  g_engine->run();

  // graceful exit
  g_engine->terminate();

  delete g_engine;
  return 0;
}

//==============================================================================
void Engine::send_event(ModuleEvent& event)
{
  for (u64 i = 0; i < m_modules.size(); ++i)
  {
    const EngineModuleMask mask = 1<<i;
    if (m_modules[i] && (mask & event.modules))
    {
      m_modules[i]->on_event(event);
    }
  }
}

//==============================================================================
void Engine::send_event(ModuleEvent&& event)
{
  auto& ref = event;
  this->send_event(ref);
}

//==============================================================================
bool Engine::init()
{
  // init the modules here..

  #define INIT_MODULE(_module_class, ...)                     \
  {                                                           \
    auto m = std::make_unique<_module_class>();               \
    if (!m || !m->init(__VA_ARGS__))                          \
    {                                                         \
      return false;                                           \
    }                                                         \
    m_module_init_order.push_back(m.get());                   \
    m_modules[_module_class::get_module_id()] = std::move(m); \
  }

  INIT_MODULE(GraphicsSystem);
  INIT_MODULE(InputSystem);

  #undef INIT_MODULE

  // post init
  this->send_event(ModuleEvent
  {
    .type = ModuleEventType::post_init,
  });
  
  return true;
}

//==============================================================================
void Engine::run()
{
  namespace eu = engine_utils;
  auto previous_time = std::chrono::high_resolution_clock::now();

  auto& input_system = this->get_module<InputSystem>();
  auto& gfx_system   = this->get_module<GraphicsSystem>();

  while (!this->should_quit())
  {
    auto current_time = std::chrono::high_resolution_clock::now();
    f32 frame_time = eu::duration_to_seconds(previous_time, current_time);
    previous_time = current_time;

    this->handle_journal_state_during_update();
    const bool replay_active = this->event_journal_active();

    // pump messages
    gfx_system.update_window_and_pump_messages();

    // override the player-specific inputs of the input system if 
    // a journal replay is active
    if (this->event_journal_active())
    {
      NC_ASSERT(m_journal);
      auto& journal_top = m_journal->frames.back();
      input_system.override_player_inputs(journal_top.player_inputs);
    }

    f32 game_logic_update_time = replay_active
      ? m_journal->frames.back().frame_time
      : frame_time;

    if (m_recorded_journal)
    {
      // store frametime and inputs into the journal we are recoring
      auto& frames = m_recorded_journal->frames;
      frames.insert(frames.begin(), EventJournalFrame
      {
        .player_inputs = input_system.get_inputs().player_inputs,
        .frame_time    = game_logic_update_time,
      });
    }

    // update
    this->send_event(ModuleEvent
    {
      .type = ModuleEventType::game_update,
      .update = {.dt = game_logic_update_time},
    });

    // render
    this->send_event(ModuleEvent
    {
      .type = ModuleEventType::render,
    });

    // cleanup
    this->send_event(ModuleEvent
    {
      .type = ModuleEventType::cleanup,
    });

    NC_TODO("We might actually want to wait here or drop a frame because the "
      "journal's framerate might be different than our own");

    if (this->event_journal_active())
    {
      // pop the last frame of the journal
      NC_ASSERT(m_journal);
      m_journal->frames.pop_back();
    }
  }
}

//==============================================================================
void Engine::terminate()
{
  this->send_event(ModuleEvent
  {
    .type = ModuleEventType::pre_terminate
  });

  auto terminate_event = ModuleEvent
  {
    .type = ModuleEventType::terminate,
  };

  // terminate modules in the opposite order of initialization
  for (auto* module : m_module_init_order | std::views::reverse)
  {
    NC_ASSERT(module);
    module->on_event(terminate_event);
  }
}

//==============================================================================
void Engine::request_quit()
{
  m_should_quit = true;
}

//==============================================================================
void Engine::install_and_replay_event_journal(EventJournal&& journal)
{
  if (this->event_journal_installed())
  {
    // the old journal was just replaying
    this->stop_event_journal();
  }

  m_journal_installed = true;
  m_journal_active   = false;

  m_journal = std::make_unique<EventJournal>(std::move(journal));
}

//==============================================================================
void Engine::stop_event_journal()
{
  m_journal_interrupted = true;
  m_journal.reset();
}

//==============================================================================
void Engine::set_recording_journal(EventJournal* journal)
{
  m_recorded_journal = journal;
}

//==============================================================================
bool Engine::should_quit() const
{
  return m_should_quit;
}

//==============================================================================
bool Engine::event_journal_installed() const
{
  return m_journal_installed;
}

//==============================================================================
bool Engine::event_journal_active() const
{
  return m_journal_active;
}

//==============================================================================
void Engine::handle_journal_state_during_update()
{
  if (m_journal_interrupted)
  {
    // the journal that was just replaying was interrupted, send a message
    this->send_event(ModuleEvent
    {
      .type = ModuleEventType::event_journal_uninstalled,
    });
    m_journal_interrupted = false;
    m_journal_active      = false;
  }

  if (m_journal_installed)
  {
    // new journal was installed, send a message
    NC_ASSERT(m_journal);
    this->send_event(ModuleEvent
    {
      .type = ModuleEventType::event_journal_installed,
    });
    m_journal_installed = false;
    m_journal_active    = true;
  }

  if (m_journal && m_journal->frames.empty())
  {
    // the journal was fully replayed, end replay
    this->send_event(ModuleEvent
    {
      .type = ModuleEventType::event_journal_uninstalled,
    });
    m_journal_interrupted = false;
    m_journal_active      = false;
    m_journal_installed   = false;
  }
}

}

