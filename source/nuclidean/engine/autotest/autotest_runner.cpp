// Project Nuclidean Source File
#include <config.h>

#if NC_TESTS

#include <engine/autotest/autotest_runner.h>

#include <common.h>
#include <unit_test.h>

#include <SDL2/include/SDL.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <regex>
#include <vector>

namespace nc::autotest
{

constexpr std::string_view TESTS_DIRECTORY = "tests";

//==============================================================================
static std::string_view get_test_name(std::string_view path)
{
  path.remove_prefix(TESTS_DIRECTORY.size() + 1);
  return path.substr(0, path.rfind('.'));
}

//==============================================================================
// Finds tests to run.
static std::vector<std::string> find_tests(const std::string& filter)
{
  const std::regex regex{filter};

  std::vector<std::string> paths;
  for (const auto& entry : std::filesystem::recursive_directory_iterator{TESTS_DIRECTORY})
  {
    if (entry.path().extension() != ".nct")
      continue;

    std::string path = entry.path().generic_string();
    const std::string_view name = get_test_name(path);
    if (std::regex_match(name.begin(), name.end(), regex))
      paths.push_back(std::move(path));
  }

  std::sort(paths.begin(), paths.end());
  return paths;
}

//==============================================================================
// Launches the game with single test.
static s32 run_test(const std::string& command, std::string& out_output)
{
  FILE* const pipe = _popen(command.c_str(), "r");
  nc_expect(pipe, "Can't run \"{}\"", command);

  for (s32 character = std::fgetc(pipe); character != EOF; character = std::fgetc(pipe))
    out_output.push_back(cast<char>(character));

  return _pclose(pipe);
}

//==============================================================================
static std::string_view get_last_output_line(std::string_view output)
{
  output = output.substr(0, output.find_last_not_of('\n') + 1);
  return output.substr(output.rfind('\n') + 1);
}

//==============================================================================
// Get crash name based on game exit code
static std::string_view get_crash_name(u32 exit_code)
{
  switch (exit_code)
  {
    case 0xC0000005: return "access violation";
    case 0xC00000FD: return "stack overflow";
    case 0xC0000374: return "heap corruption";
    case 0xC0000409: return "fast fail";
    case 0xC0000094: return "integer division by zero";
    case 0x80000003: return "breakpoint";
    case 3:          return "abort";
  }

  return "unknown crash";
}

//==============================================================================
s32 run_tests(std::string_view launch_path, const std::string& filter)
{
  if (!std::filesystem::is_directory(TESTS_DIRECTORY))
  {
    nc_crit("Tests directory \"{}\" is missing.", TESTS_DIRECTORY);
    return 1;
  }

  const std::vector<std::string> test_paths = find_tests(filter);
  const u64 test_count = test_paths.size();
  std::cout << std::format("[Autotest] Running {} tests. Match filter: \"{}\"", test_count, filter) << std::endl;

  // current directory may have changed since launch
  const std::unique_ptr<char, decltype(&SDL_free)> base_path{SDL_GetBasePath(), &SDL_free};
  const std::filesystem::path executable = std::filesystem::path{base_path.get()} / std::filesystem::path{launch_path}.filename();

  u64 success_count = 0;
  for (const std::string& path : test_paths)
  {
    const std::string command = std::format("\"\"{}\" {} \"{}\" 2>&1\"", executable.string(), SCRIPT_ARG, path);

    std::string output;
    const s32 exit_code = run_test(command, output);

    const std::string_view name = get_test_name(path);
    if (exit_code == 0)
    {
      ++success_count;
      std::cout << std::format("[SUCCESS] {}", name) << std::endl;
    }
    else if (exit_code == FAIL_EXIT_CODE)
    {
      std::cout << std::format("[ FAIL  ] {}: {}", name, get_last_output_line(output)) << std::endl;
    }
    else
    {
      // TODO: consider running the test as a debuggee, so crashes it can't report itself get a location too
      const u32 crash_code = cast<u32>(exit_code);
      std::cout << std::format("[ FAIL  ] {}: crashed: {} (0x{:08X})", name, get_crash_name(crash_code), crash_code)
        << std::endl;
    }
  }

  const u64 success_rate = test_count ? success_count * 100 / test_count : 100;
  std::cout << std::format
  (
    "[Autotest] {} tests out of {} successful. Success rate: {}%", success_count, test_count, success_rate
  ) << std::endl;

  return success_count == test_count ? 0 : 1;
}

// Unit tests

//==============================================================================
static bool autotest_runner_test_name(unit_test::TestCtx& /*ctx*/)
{
  NC_TEST_ASSERT(get_test_name("tests/smoke/walk_forward.nct") == "smoke/walk_forward");
  NC_TEST_ASSERT(get_test_name("tests/v1.2/level.start.nct") == "v1.2/level.start");

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(autotest_runner_test_name);

//==============================================================================
static bool autotest_runner_last_output_line(unit_test::TestCtx& /*ctx*/)
{
  NC_TEST_ASSERT(get_last_output_line("loading level\nfailed: condition is false\n") == "failed: condition is false");
  NC_TEST_ASSERT(get_last_output_line("first\nlast\n\n\n") == "last");
  NC_TEST_ASSERT(get_last_output_line("only line") == "only line");
  NC_TEST_ASSERT(get_last_output_line("").empty());
  NC_TEST_ASSERT(get_last_output_line("\n\n").empty());

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(autotest_runner_last_output_line);

}

#endif
