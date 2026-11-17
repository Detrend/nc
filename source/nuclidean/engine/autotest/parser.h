// Project Nuclidean Source File
#pragma once

#include <engine/autotest/commands.h>

#include <string>

namespace nc::autotest
{

// Parses a test script
bool parse_script(const std::string& path, Script& out_script, std::string& out_error);

}
