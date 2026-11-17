// Project Nuclidean Source File
#pragma once

#include <types.h>

#include <string>
#include <string_view>

namespace nc::autotest
{

// Argument which launches the game with given test script and exit when test finishes
constexpr std::string_view SCRIPT_ARG = "-autotest_script";
// Exit code for failed test. Failed test prints the reason as its last output line
constexpr s32 FAIL_EXIT_CODE = 1;

// Runs each test script matching the filter in its own game process, returns the exit code
s32 run_tests(std::string_view launch_path, const std::string& filter);

}
