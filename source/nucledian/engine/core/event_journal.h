// Project Nucledian Source File
#pragma once

#include <engine/input/game_input.h>

#include <vector>

namespace nc
{
  
struct EventJournalFrame
{
  PlayerSpecificInputs player_inputs;
  f32                  frame_time;
};

struct EventJournal
{
  std::vector<EventJournalFrame> frames;
};

}

