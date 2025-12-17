// Project Nucledian Source File
#pragma once

// MR says: Try to not include anything here as the engine is included by
// pretty much everything and therefore would make the include chain too deep
// and cause longer builds.
// Instead forward declare everything and do the includes in the .cpp file.

#include <engine/core/is_engine_module.h>

#include <array>
#include <memory>
#include <vector>
#include <string>

union SDL_Event;

namespace nc
{

struct ModuleEvent;
class  IEngineModule;
struct MapSectors;

class Engine
{
public:
  template<typename Module>
    requires IsEngineModule<Module>
  Module& get_module();

  void send_event(ModuleEvent& event);
  void send_event(ModuleEvent&& event);

  bool init();
  void run();
  void terminate();

  const MapSectors& get_map();

  // TODO: if we use this in multiple places then
  // probably introduce a quit reason enum as well
  void request_quit();

  void process_window_event(const SDL_Event& evnt);

  f32  get_delta_time() const;

  u64  get_frame_idx() const;

  void pause(bool pause);

private:
  bool should_quit()             const;
  void build_map_and_sectors();

private:
  using ModuleArray = std::array<std::unique_ptr<IEngineModule>, 8>;
  using ModuleVector = std::vector<IEngineModule*>;

private:
  ModuleArray   m_modules;
  ModuleVector  m_module_init_order;
  f32           m_delta_time = 0.0f; // last frame time in seconds
  u64           m_frame_idx = 0;     // index of a frame, currently only for debug
  bool          m_should_quit : 1 = false;

  std::unique_ptr<MapSectors> m_map = nullptr;
};

Engine& get_engine();

int init_engine_and_run_game(const std::vector<std::string>& args);

}

#include <engine/core/engine.inl>
