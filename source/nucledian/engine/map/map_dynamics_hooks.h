// Project Nucledian Source File
#pragma once

#include <types.h>
#include <engine/map/map_types.h>
#include <engine/entity/entity_types.h>

#include <map>
#include <functional>
#include <string>
#include <memory>

#include <engine/map/map_dynamics.h>
#include <engine/player/level_types.h>
#include <engine/game/game_system.h>


namespace nc
{

  std::unique_ptr<IActivatorHook> create_hook_by_type(const std::string& hook_type);


  class ActivatorHook_LevelTransition : public IActivatorHook {
  public:

    virtual void load(const SerializedData& data);

    virtual void on_activated_start() override;

  private:
    LevelName destination;
  };

}
