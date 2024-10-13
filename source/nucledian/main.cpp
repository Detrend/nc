// Project Nucledian Source File
#include <config.h>
#include <engine/core/engine.h>
#include <SDL2/include/SDL.h>
#include <vector_maths.h>

#include <vector>
#include <string>

int main(int argc, char* args[])
{
  std::vector<std::string> command_line_args(args, args+argc);
  return nc::init_engine_and_run_game(command_line_args);
}

