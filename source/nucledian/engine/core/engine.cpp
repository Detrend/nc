// Project Nucledian Source File
#include <common.h>
#include <config.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module.h>
#include <engine/core/module_event.h>
#include <engine/core/is_engine_module.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/event_journal.h>

#include <cvars.h>

#include <engine/graphics/graphics_system.h>
#include <engine/input/input_system.h>
#include <engine/player/thing_system.h>
#include <engine/sound/sound_system.h>
#include <engine/tweens/tween_system.h>
#include <engine/map/map_system.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/sector_mapping.h>

#ifdef NC_PROFILING
#include <benchmark/benchmark.h>
#include <algorithm>
#endif

#ifdef NC_TESTS
#include <unit_test.h>
#include <format>
#include <iostream>
#include <regex>
#endif

#include <ranges>  // std::views::reverse
#include <chrono>  // std::chrono::high_resolution_clock
#include <cstdlib> // std::rand


namespace nc
{

static Engine* g_engine = nullptr;

//==============================================================================
namespace engine_utils
{

[[maybe_unused]] constexpr cstr BENCHMARK_ARG      = "-benchmark";
[[maybe_unused]] constexpr cstr UNIT_TEST_ARG      = "-unit_test";
[[maybe_unused]] constexpr cstr TEST_FILTER_PREFIX = "-test_filter=";

//==============================================================================
static f32 duration_to_seconds(auto t1, auto t2)
{
  using namespace std::chrono;
  nc_assert(t2 >= t1);

  return duration_cast<microseconds>(t2 - t1).count() / 1'000'000.0f;
}

//==============================================================================
#ifdef NC_BENCHMARK
static bool execute_benchmarks_if_required(const std::vector<std::string>& args)
{
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
#ifdef NC_TESTS
//==============================================================================
static std::string find_text_filter_regex(const std::vector<std::string>& args)
{
  const u64 PREFIX_SIZE = std::strlen(TEST_FILTER_PREFIX);

  auto match_filter = std::string{".*"};

  // search for the string prefix
  auto it = std::find_if(args.begin(), args.end(), [&](const std::string& arg)
  {
    return arg.find(TEST_FILTER_PREFIX) == 0 && arg.size() > PREFIX_SIZE;
  });

  // change the match filter if the prefix was found
  if (it != args.end())
  {
    match_filter = std::string{it->begin() + PREFIX_SIZE, it->end()};
  }

  return match_filter;
}

//==============================================================================
static bool execute_unit_tests_if_required(const std::vector<std::string>& args)
{
  auto test_arg_it = std::find(args.begin(), args.end(), UNIT_TEST_ARG);
  if (test_arg_it == args.end())
  {
    // do not run tests if the appropriate cmd line argument was not set
    return false;
  }

  const auto  match_filter_str = find_text_filter_regex(args);
  const auto& test_list = unit_test::get_tests();

  std::regex match_filter = std::regex{match_filter_str};

  u64 total_test_cnt = 0;
  u64 ok_test_cnt    = 0;

  std::cout << std::format
  (
    "[Unit Tests] Running tests. Match filter: \"{}\"\n", match_filter_str
  );

  for (const auto& test : test_list)
  {
    nc_assert(test.test_name && test.test_function, "Bad test name or func!");

    if (!std::regex_match(test.test_name, match_filter))
    {
      // this test does not match the test filter
      continue;
    }

    unit_test::TestCtx context;
    context.argument = test.argument_value;

    const bool ok = test.test_function(context);

    ok_test_cnt    += ok;
    total_test_cnt += 1;

    std::cout << std::format
    (
      "[{}] {}\n", ok ? "SUCCESS" : " FAIL  ", test.test_name
    );
  }

  u64 ok_perc = total_test_cnt ? (ok_test_cnt * 100) / total_test_cnt : 100;

  std::cout << std::format
  (
    "[Unit Tests] {} tests out of {} successful. Success rate: {}%\n",
    ok_test_cnt, total_test_cnt, ok_perc
  );

  return true;
}
#endif

}

//==============================================================================
Engine& get_engine()
{
  nc_assert(g_engine);
  return *g_engine;
}

//==============================================================================
int init_engine_and_run_game([[maybe_unused]]const std::vector<std::string>& args)
{
  [[maybe_unused]] bool exit_after_benchmarks_and_tests = false;

  #ifdef NC_BENCHMARK
  if (engine_utils::execute_benchmarks_if_required(args))
  {
    // only benchmark run, exit
    exit_after_benchmarks_and_tests = true;
  }
  #endif

  #ifdef NC_TESTS
  if (engine_utils::execute_unit_tests_if_required(args))
  {
    // running only tests, exit
    exit_after_benchmarks_and_tests = true;
  }
  #endif

  if (exit_after_benchmarks_and_tests)
  {
    // exit if we want to run only tests or benchmarks
    return 0;
  }

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
  INIT_MODULE(ThingSystem);
  INIT_MODULE(SoundSystem);
  INIT_MODULE(TweenSystem);

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

  while (!this->should_quit())
  {
    auto current_time = std::chrono::high_resolution_clock::now();
    f32 frame_time = eu::duration_to_seconds(previous_time, current_time);

    // Limit the FPS if desired
    const f32 min_frame_time = CVars::has_fps_limit ? 1.0f / CVars::fps_limit : 0.0f;
    while (frame_time < min_frame_time)
    {
      // Spin until the time runs out
      current_time = std::chrono::high_resolution_clock::now();
      frame_time   = eu::duration_to_seconds(previous_time, current_time);
    }

    previous_time = current_time;
    m_delta_time  = frame_time;

    // notify frame start
    this->send_event(ModuleEvent
    {
      .type = ModuleEventType::frame_start,
    });

    this->handle_journal_state_during_update();
    const bool replay_active = this->event_journal_active();

    // pump messages
    input_system.update_window_and_pump_messages();

    // override the player-specific inputs of the input system if 
    // a journal replay is active
    if (this->event_journal_active())
    {
      nc_assert(m_journal);
      auto& journal_top = m_journal->frames.back();
      input_system.override_player_inputs(journal_top.player_inputs);
    }

    const f32 game_logic_update_time = replay_active
      ? m_journal->frames.back().frame_time // take time speed from replay
      : frame_time * CVars::time_speed;     // modify time speed by cvar

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

    // frame start
    this->send_event(ModuleEvent
    {
      .type = ModuleEventType::frame_start,
    });

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

    NC_TODO("This is a quite stupid implementation of the journal.. "
      "Instead of manipulating the journal and popping the frames we "
      "could just keep an iterator are move it forward.");

    if (this->event_journal_active())
    {
      // pop the last frame of the journal
      nc_assert(m_journal);
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
    nc_assert(module);
    module->on_event(terminate_event);
  }
}

//==============================================================================
const MapSectors& Engine::get_map()
{
  return ThingSystem::get().get_map();
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
  m_journal_active    = false;

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
void Engine::process_window_event(const SDL_Event& event)
{
  switch (event.type)
  {
    case SDL_QUIT:
    {
      this->request_quit();
      break;
    }

    #ifdef NC_DEBUG_DRAW
    case SDL_KEYDOWN:
    {
      if (event.key.keysym.scancode == SDL_SCANCODE_F1)
      {
        CVars::time_speed = CVars::time_speed ? 0.0f : 1.0f;
      }

      if (event.key.keysym.scancode == SDL_SCANCODE_F4)
      {
        CVars::enable_top_down_debug = !CVars::enable_top_down_debug;
      }

      if (event.key.keysym.scancode == SDL_SCANCODE_GRAVE)
      {
        CVars::display_debug_window = !CVars::display_debug_window;
      }
      break;
    }
    #endif
  }
}

//==============================================================================
f32 Engine::get_delta_time()
{
  return m_delta_time;
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
    nc_assert(m_journal);
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

