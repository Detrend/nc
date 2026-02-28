// Project Nucledian Source File
#include <common.h>
#include <config.h>

#include <cvars.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module.h>
#include <engine/core/module_event.h>
#include <engine/core/is_engine_module.h>
#include <engine/core/engine_module_types.h>

#include <engine/map/map_system.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/sector_mapping.h>

// The engine subsystems
#include <engine/graphics/graphics_system.h>
#include <engine/input/input_system.h>
#include <engine/game/game_system.h>
#include <engine/sound/sound_system.h>
#include <engine/ui/user_interface_module.h>

#ifdef NC_BENCHMARK
#include <benchmark/benchmark.h>
#include <algorithm>
#endif

#include <profiling.h>

#ifdef NC_TESTS
#include <unit_test.h>
#include <format>
#include <iostream>
#include <regex>
#endif

#include <SDL.h>   // SDL_Event

#include <ranges>   // std::views::reverse
#include <chrono>   // std::chrono::high_resolution_clock
#include <cstdlib>  // std::rand
#include <iterator> // std::next

namespace nc
{

static Engine* g_engine = nullptr;

//==============================================================================
namespace engine_utils
{

[[maybe_unused]] constexpr cstr BENCHMARK_ARG      = "-benchmark";
[[maybe_unused]] constexpr cstr UNIT_TEST_ARG      = "-unit_test";
[[maybe_unused]] constexpr cstr TEST_FILTER_PREFIX = "-test_filter=";
[[maybe_unused]] constexpr cstr START_LEVEL_ARG    = "-start_level";
[[maybe_unused]] constexpr cstr START_DEMO_ARG     = "-start_demo";

//==============================================================================
static f32 duration_to_seconds(auto t1, auto t2)
{
  using namespace std::chrono;
  nc_assert(t2 >= t1);

  return duration_cast<microseconds>(t2 - t1).count() / 1'000'000.0f;
}

//==============================================================================
static void limit_min_frametime(f32& frame_time)
{
  // 20 FPS at least during debugging
	constexpr f32 DEFAULT_MIN = 20.0f;

	f32 min_fps  = CVars::has_min_fps ? CVars::fps_min : DEFAULT_MIN;
  f32 min_time = 1.0f / min_fps;

  frame_time = std::min(frame_time, min_time);
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
int init_engine_and_run_game(const CmdArgs& args)
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

  if (!g_engine->init(args))
  {
    // failed to init, end it here
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

using CmdArgs = std::vector<std::string>;

//==============================================================================
static bool contains_pair_of_args
(
  const CmdArgs& cmd_args, cstr search_for, std::string& out
)
{
  auto it = std::find_if(cmd_args.begin(), cmd_args.end(),
  [&](const std::string& arg)
  {
    return arg == search_for;
  });

  if (it == cmd_args.end())
  {
    return false;
  }

  if (std::next(it) == cmd_args.end())
  {
    return false;
  }

  out = *std::next(it);
  return true;
}

//==============================================================================
static bool should_play_demo(const CmdArgs& cmd_args, std::string& out_demo)
{
  return contains_pair_of_args(cmd_args, engine_utils::START_DEMO_ARG, out_demo);
}

//==============================================================================
static bool should_play_level(const CmdArgs& cmd_args, std::string& out_lvl)
{
  return contains_pair_of_args(cmd_args, engine_utils::START_LEVEL_ARG, out_lvl);
}

//==============================================================================
bool Engine::init(const CmdArgs& cmd_args)
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
  INIT_MODULE(GameSystem);
  INIT_MODULE(SoundSystem);
  INIT_MODULE(UserInterfaceSystem);

  #undef INIT_MODULE

  // post init
  this->send_event(ModuleEvent
  {
    .type = ModuleEventType::post_init,
  });

  GameSystem&          game_system = get_module<GameSystem>();
  UserInterfaceSystem& ui_system   = get_module<UserInterfaceSystem>();

  if (std::string demo; should_play_demo(cmd_args, demo))
  {
    // Play one demo and then exit
    std::string lvl_name;
    std::vector<DemoDataFrame> frames;

    if (!load_demo_from_file(demo, lvl_name, frames))
    {
      return false;
    }

    m_game_state = GameState::debug_playing_demo;

    game_system.request_level_change
    (
      LevelName{std::string_view{lvl_name}},
      std::move(frames)
    );
  }
  else if (std::string lvl; should_play_level(cmd_args, lvl))
  {
    // Start a level
    m_game_state = GameState::player_handled;
    game_system.request_play_level(LevelName{std::string_view{lvl}});
  }
  else
  {
    // Open up the menu and play demos on the background
    m_game_state = GameState::in_menu;
    ui_system.get_menu_manager()->set_visible(true);
    this->play_random_demo();
  }

  return true;
}

//==============================================================================
void Engine::play_random_demo()
{
  GameSystem& game_system = GameSystem::get();

  std::vector<std::string> demos = list_available_demo_files();

  if (demos.empty())
  {
    // Empty level = black screen in the menu
    nc_warn("No demos available, playing empty level.");
    game_system.request_empty_level();
    return;
  }

  // Pick a random one
  const std::string& demo = demos[std::rand() % demos.size()];

  // Play one demo and then exit
  std::string lvl_name;
  std::vector<DemoDataFrame> frames;

  if (!load_demo_from_file(demo, lvl_name, frames))
  {
    // Empty level = black screen in the menu
    nc_warn(
      "Failed to play a random demo from a file \"{}\", starting empty level.",
      demo);
    game_system.request_empty_level();
    return;
  }

  // Demo loaded, play it
  game_system.request_level_change
  (
    LevelName{std::string_view{lvl_name}},
    std::move(frames)
  );
}

//==============================================================================
void Engine::run()
{
  namespace eu = engine_utils;
  auto previous_time = std::chrono::high_resolution_clock::now();

  InputSystem& input_system = this->get_module<InputSystem>();

  while (!this->should_quit())
  {
    auto current_time = std::chrono::high_resolution_clock::now();
    f32 frame_time = eu::duration_to_seconds(previous_time, current_time);
#ifdef NC_PROFILING
    Profiler::get().new_frame(m_frame_idx, frame_time);
    NC_SCOPE_PROFILER(FullFrame)
#endif
    eu::limit_min_frametime(frame_time);

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

    // pump messages
    input_system.update_window_and_pump_messages();

    const f32 game_logic_update_time = frame_time * CVars::time_speed;

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

    m_frame_idx += 1;
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
  return GameSystem::get().get_map();
}

//==============================================================================
void Engine::request_quit()
{
  m_should_quit = true;
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
        //CVars::time_speed = CVars::time_speed ? 0.0f : 1.0f;
        //pause(CVars::time_speed ? 0.0f : 1.0f);
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
f32 Engine::get_delta_time() const
{
  return m_delta_time;
}

//==============================================================================
u64 Engine::get_frame_idx() const
{
  return m_frame_idx;
}

//==============================================================================
void Engine::pause(bool pause)
{
  CVars::time_speed = pause ? 0.0f : 1.0f;
}

//==============================================================================
void Engine::on_demo_end()
{
  switch (m_game_state)
  {
    case GameState::in_menu:
    {
      // Schedule the next demo. This overwrites the requests of other systems
      this->play_random_demo();
    }
    break;

    case GameState::debug_playing_demo:
    {
      // Exit the game.. Will quit at the start of the next frame
      m_should_quit = true;
    }
    break;
  }
}

//==============================================================================
void Engine::on_level_end()
{
  // Do nothing
}

//==============================================================================
bool Engine::should_quit() const
{
  return m_should_quit;
}

}
