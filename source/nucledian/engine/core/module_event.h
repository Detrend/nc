// Project Nucledian Source File
#pragma once

#include <engine/core/engine_module_id.h>
#include <engine/player/level_types.h>

#include <types.h>

namespace nc
{

namespace ModuleEventType
{
  enum value : ModuleEventId
  {
    invalid = 0,

    post_init,           // sent to all modules after initialization

    frame_start,         // a new frame has just started - before everything else
    game_update,         // during gameplay
    render,

    before_map_rebuild,  // send before the map rebuild starts
    after_map_rebuild,   // sent after the map is rebuilt

    cleanup,             // at the end of a frame

    menu_opened,         // the menu was opened
    menu_closed,         // the menu was closed

    new_game_level_requested,

    pre_terminate,
    terminate,           // terminate the module
    // -- //
    count
  };
}

// for game update and other updates that require only the delta time
struct EventUpdate
{
  f32 dt;
};

struct EventCleanup
{
  
};

struct EventNewGame
{
  LevelName level;
};

struct ModuleEvent
{
  ModuleEventId    type    = ModuleEventType::invalid; // the type of the message
  EngineModuleMask modules = MASK_ALL_MODULES;         // mask - which modules should receive the event
  union
  {
    EventUpdate  update;
    EventCleanup cleanup;
    EventNewGame new_game;
  };
};

}
