// Project Nuclidean Source File
#pragma once

#include <engine/core/engine_module_id.h>
#include <engine/network/constants.h>
#include <engine/player/level_types.h>

#include <types.h>

namespace nc
{

namespace ModuleEventType
{
  enum value : ModuleEventId
  {
    invalid = 0,

    post_init,                 // sent to all modules after initialization

    frame_start,               // a new frame has just started - before everything else
    game_update,               // during gameplay
    render,

    before_map_rebuild,        // send before the map rebuild starts
    after_map_rebuild,         // sent after the map is rebuilt

    cleanup,                   // at the end of a frame

    menu_opened,               // the menu was opened
    menu_closed,               // the menu was closed

    new_game_level_requested,
    next_level_requested,      // player clicks "next" on transition screen
    main_menu_requested,       // player clicks "menu" on transition screen

    demo_ended,                // when the demo simulated in the game system ends
    level_ended,               // when the level simulated in the game system ends

    net_player_spawned,        // remote player joined
    net_player_despawned,      // remote player left
    net_input_arrived,         // all-player inputs for this tick

    pre_terminate,
    terminate,                 // terminate the module
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

struct EventNetPlayer
{
  PlayerID player_id;
};

struct EventNetInputs
{
  const InputArray* inputs;
};

struct ModuleEvent
{
  ModuleEventId    type    = ModuleEventType::invalid; // the type of the message
  EngineModuleMask modules = MASK_ALL_MODULES;         // mask - which modules should receive the event
  union
  {
    EventUpdate    update;
    EventCleanup   cleanup;
    EventNewGame   new_game;
    EventNetPlayer net_player;
    EventNetInputs net_inputs;
  };
};

}
