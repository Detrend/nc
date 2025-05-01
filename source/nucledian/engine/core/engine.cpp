// Project Nucledian Source File
#include <common.h>
#include <config.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module.h>
#include <engine/core/module_event.h>
#include <engine/core/is_engine_module.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/event_journal.h>

#include <engine/entities.h>
#include <cvars.h>

#include <engine/graphics/graphics_system.h>
#include <engine/input/input_system.h>
#include <engine/player/thing_system.h>
#include <engine/map/map_system.h>

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

  #undef INIT_MODULE

  // MR says: this is temporary
  this->build_map_and_sectors();

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
    const f32 frame_time = eu::duration_to_seconds(previous_time, current_time);
    previous_time = current_time;

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
static void test_make_sector(
  const std::vector<u16>&                     points,
  std::vector<map_building::SectorBuildData>& out,
  int                                         portal_wall_id    = -1,
  WallRelID                                   portal_wall_id_to = 0,
  SectorID                                    portal_sector     = INVALID_SECTOR_ID)
{
  std::vector<map_building::WallBuildData> walls;

  for (int i = 0; i < static_cast<int>(points.size()); ++i)
  {
    auto p = points[i];
    bool is_portal = portal_sector != INVALID_SECTOR_ID && portal_wall_id == i;
    walls.push_back(map_building::WallBuildData
    {
      .point_index            = p,
      .nc_portal_point_index  = is_portal ? portal_wall_id_to : INVALID_WALL_REL_ID,
      .nc_portal_sector_index = is_portal ? portal_sector     : INVALID_SECTOR_ID,
    });
  }

  out.push_back(map_building::SectorBuildData
  {
    .points = std::move(walls),
  });
}

//==============================================================================
[[maybe_unused]] static void make_cool_looking_map(MapSectors& map)
{
  // {22, 12, 13, 23}
  std::vector<vec2> points =
  {
    vec2{0	, 0},
    vec2{6	, 1},
    vec2{7	, 5},
    vec2{9	, 5},
    vec2{11	, 4},
    vec2{14	, 4},
    vec2{16	, 5},
    vec2{16	, 8},
    vec2{14	, 9},
    vec2{11	, 9},
    vec2{9	, 8},
    vec2{8	, 8},
    vec2{7	, 9},
    vec2{7	, 12},
    vec2{6	, 13},
    vec2{5	, 13},
    vec2{4	, 12},
    vec2{4	, 9},
    vec2{4	, 6},
    vec2{2	, 4},
    vec2{3	, 1},
    vec2{5	, 9},
    vec2{6	, 9},
    vec2{6	, 10},
    vec2{5	, 10},
    vec2{16	, 14},    // extra 4 pts
    vec2{18	, 14},
    vec2{18	, 18},
    vec2{16	, 18},
  };

  //constexpr vec2 max_range = {20.0f, 20.0f};
  // normalize the points
  //for (auto& pt : points)
  //{
  //  pt = ((pt / max_range) * 2.0f) - vec2{1.0f};
  //}

  // and then the sectors
  std::vector<map_building::SectorBuildData> sectors;

  test_make_sector({1, 2, 18, 19, 20}, sectors);
  test_make_sector({18, 2, 11, 12, 22, 21, 17}, sectors);
  test_make_sector({2, 3, 10, 11}, sectors);
  test_make_sector({3, 4, 5, 6, 7, 8, 9, 10}, sectors);
  test_make_sector({22, 12, 13, 23}, sectors, 1, 0, 8);    // this one
  test_make_sector({23, 13, 14, 15, 16, 24}, sectors);
  test_make_sector({17, 21, 24, 16}, sectors);
  test_make_sector({21, 22, 23, 24}, sectors);
  test_make_sector({25, 26, 27, 28}, sectors, 0, 1, 4);

  using namespace map_building::MapBuildFlag;

  map_building::build_map(
    points, sectors, map,
    //omit_convexity_clockwise_check |
    //omit_sector_overlap_check      |
    //omit_wall_overlap_check        |
    assert_on_fail);
}

//==============================================================================
[[maybe_unused]]static void make_random_square_maze_map(MapSectors& map, u32 size, u32 seed)
{
  constexpr f32 SCALING = 20.0f;

  std::srand(seed);

  std::vector<vec2> points;

  // first up the points
  for (u32 i = 0; i < size; ++i)
  {
    for (u32 j = 0; j < size; ++j)
    {
      const f32 x = ((j / static_cast<f32>(size-1)) * 2.0f - 1.0f) * SCALING;
      const f32 y = ((i / static_cast<f32>(size-1)) * 2.0f - 1.0f) * SCALING;
      points.push_back(vec2{x, y});
    }
  }

  // and then the sectors
  std::vector<map_building::SectorBuildData> sectors;

  for (u16 i = 0; i < size-1; ++i)
  {
    for (u16 j = 0; j < size-1; ++j)
    {
      auto rng = std::rand();
      if (rng % 4 != 0)
      {
        u16 i1 = static_cast<u16>(i * size + j);
        u16 i2 = i1+1;
        u16 i3 = static_cast<u16>((i+1) * size + j + 1);
        u16 i4 = i3-1;
        test_make_sector({i1, i2, i3, i4}, sectors);
      }
    }
  }

  using namespace map_building::MapBuildFlag;

  map_building::build_map(
    points, sectors, map,
    //omit_convexity_clockwise_check |
    //omit_sector_overlap_check      |
    //omit_wall_overlap_check        |
    assert_on_fail);
}

//==============================================================================
[[maybe_unused]] static void make_demo_map(MapSectors& map)
{
  std::vector<vec2> points =
  {
    vec2{0, 0},
    vec2{9, 0},
    vec2{9, 9},
    vec2{0, 9}, //
    vec2{2, 2},
    vec2{7, 2},
    vec2{7, 7},
    vec2{2, 7}, //
    vec2{3, 2},
    vec2{6, 2},
    vec2{7, 3},
    vec2{7, 6},
    vec2{6, 7},
    vec2{3, 7},
    vec2{2, 6},
    vec2{2, 3},  //
    vec2{13, 9}, // 16
    vec2{16, 9},
    vec2{16, 2},
    vec2{23, 2},
    vec2{23, 9},
    vec2{26, 9},
    vec2{26, 12},
    vec2{20, 12},
    vec2{20, 9},
    vec2{19, 9},
    vec2{19, 12},
    vec2{13, 12},
    vec2{19, 5},
    vec2{20, 5},
    vec2{20, 6},
    vec2{19, 6},
  };

  // and then the sectors
  std::vector<map_building::SectorBuildData> sectors;

  test_make_sector({0, 1, 5, 9,  8,  4}, sectors);
  test_make_sector({1, 2, 6, 11, 10, 5}, sectors, 3, 4, 9);
  test_make_sector({2, 3, 7, 13, 12, 6}, sectors);
  test_make_sector({3, 0, 4, 15, 14, 7}, sectors, 3, 4, 4);

  test_make_sector({16, 17, 25, 26, 27}, sectors, 4, 3, 3); // 4
  test_make_sector({25, 17, 31, 30, 20, 24}, sectors);
  test_make_sector({31, 17, 18, 28}, sectors);
  test_make_sector({18, 19, 29, 28}, sectors);
  test_make_sector({19, 20, 30, 29}, sectors);
  test_make_sector({22, 23, 24, 20, 21}, sectors, 4, 3, 1);

  using namespace map_building::MapBuildFlag;

  map_building::build_map(
    points, sectors, map,
    //omit_convexity_clockwise_check |
    //omit_sector_overlap_check      |
    //omit_wall_overlap_check        |
    assert_on_fail);
}

//==============================================================================
[[maybe_unused]] static void make_simple_portal_test_map(MapSectors& map)
{
  /*
  *     0-------1-------2
  *     |               |
  *     A               | <- sector 0
  *     |               |   
  *     3-------4   -   5------12
  *             |               |
  * sector 1 -> B       |       B <- sector 3
  *             |               |
  *     6-------7   -   8------13
  *     |               |
  *     A               | <- sector 2
  *     |               |
  *     9------10------11
  * 
  * note: its actually flip along horizontal axis
  */

  const vec2 offset = vec2(-1.0f, 0.5f);
  std::vector<vec2> points = {
    vec2(0, 0), //  0
    vec2(1, 0), //  1
    vec2(2, 0), //  2
    vec2(0, 1), //  3
    vec2(1, 1), //  4
    vec2(2, 1), //  5
    vec2(0, 2), //  6
    vec2(1, 2), //  7
    vec2(2, 2), //  8
    vec2(0, 3), //  9
    vec2(1, 3), // 10
    vec2(2, 3), // 11
    vec2(3, 1), // 12
    vec2(3, 2), // 13
  };

  for (auto& point : points)
  {
    point += offset;
  }

  std::vector<map_building::SectorBuildData> sectors;

  // sector 0
  test_make_sector({ 0, 1, 2, 5, 4, 3 }, sectors, 5, 5, 2);
  // sector 1
  test_make_sector({ 4, 5, 8, 7 }, sectors, 3, 1, 3);
  // sector 2
  test_make_sector({ 6, 7, 8, 11, 10, 9 }, sectors, 5, 5, 0);
  // sector 3
  test_make_sector({ 5, 12, 13, 8 }, sectors, 1, 3, 1);

  using namespace map_building::MapBuildFlag;

  map_building::build_map(
    points, sectors, map,
    //omit_convexity_clockwise_check |
    //omit_sector_overlap_check      |
    //omit_wall_overlap_check        |
    assert_on_fail);
}

//==============================================================================
void Engine::build_map_and_sectors()
{
  m_map = std::make_unique<MapSectors>();
  //make_random_square_maze_map(*m_map, 32, 0);
  //make_cool_looking_map(*m_map);
  //make_simple_portal_test_map(*m_map);
  make_demo_map(*m_map);

  // geometry dump
  #if 0
  for (SectorID sid = 0; sid < m_map->sectors.size(); ++sid)
  {
    std::cout << std::format("# Start of sector {}", sid) << std::endl;

    std::vector<vec3> vertices;
    m_map->sector_to_vertices(sid, vertices);
    nc_assert((vertices.size() % 6) == 0);

    // dump them out
    for (u64 i = 0; i < vertices.size(); i += 6)
    {
      const auto v0 = vertices[i];
      const auto v1 = vertices[i+2];
      const auto v2 = vertices[i+4];
      std::cout
        << std::format(
          "[{:3f}, {:3f}, {:3f}], [{:3f}, {:3f}, {:3f}], [{:3f}, {:3f}, {:3f}],",
          v0.x, v0.y, v0.z,
          v1.x, v1.y, v1.z,
          v2.x, v2.y, v2.z)
        << std::endl;
    }
  }
  #endif
}

//==============================================================================
MapSectors& Engine::get_map()
{
  nc_assert(m_map);
  return *m_map;
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

