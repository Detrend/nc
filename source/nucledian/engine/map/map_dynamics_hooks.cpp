// Project Nucledian Source File

#include <engine/map/map_dynamics_hooks.h>
#include <engine/map/map_system.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_types.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>

#include <math/lingebra.h>
#include <math/utils.h>

#include <algorithm>

#include <json/json.hpp>


namespace nc
{

  std::unique_ptr<IActivatorHook> create_hook_by_type(const std::string &hook_type)
  {
    if (hook_type == "level_transition") {
      return std::make_unique<ActivatorHook_LevelTransition>();
    }
    return nullptr;
  }


  void ActivatorHook_LevelTransition::load(const SerializedData& data) {
    this->destination = std::string_view(data["destination"]);
  }
  void ActivatorHook_LevelTransition::on_activated_start() {
    GameSystem::get().request_level_change(destination);
  }

}
