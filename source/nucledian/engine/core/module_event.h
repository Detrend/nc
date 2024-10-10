// Project Nucledian Source File
#pragma once

#include <engine/core/engine_module_id.h>

#include <types.h>

namespace nc
{

namespace ModuleEventType
{
  enum value : ModuleEventId
  {
    invalid = 0,

    post_init,        // sent to all modules after initialization

    game_update,      // during gameplay
    paused_update,    // during paused game
    loading_update,   // during loading
    render,

    editor_update,    // during editor
    editor_render,    //

    cleanup,          // at the end of a frame

    pre_terminate,
    terminate,        // terminate the module
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

struct ModuleEvent
{
  ModuleEventId    type    = ModuleEventType::invalid; // the type of the message
  EngineModuleMask modules = MASK_ALL_MODULES;         // mask - which modules should receive the event
  union
  {
    EventUpdate  update;
    EventCleanup cleanup;
  };
};

}

