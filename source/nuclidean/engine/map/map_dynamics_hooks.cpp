// Project Nuclidean Source File

#include <engine/core/engine.h>
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

    if (hook_type == "secret")
    {
      return std::make_unique<ActivatorHook_Secret>();
    }

    return nullptr;
  }


  void ActivatorHook_LevelTransition::load(const SerializedData& data) 
  {
    this->destination = std::string_view(data["destination"]);
  }

  void ActivatorHook_LevelTransition::on_activated_start([[maybe_unused]] const ActivatorHookArg& args)
  {
    GameSystem& gs = GameSystem::get();
    gs.end_level_and_go_to_another_one_from_gamemode(destination);
  }


  void ActivatorHook_Jumppad::load(const SerializedData& data) 
  {
    const auto& vec_js = data["direction"];
    this->direction = vec3(vec_js[0], vec_js[2], vec_js[1]);
  }
  void ActivatorHook_Jumppad::on_activated_start(const ActivatorHookArg& args)
  {
    for (const auto& entity_id : args.entities) {
      Entity *const entity = GameSystem::get().get_entities().get_entity(entity_id);
      (void)entity;
    }
  }

  void ActivatorHook_Secret::load([[maybe_unused]] const SerializedData& args)
  {
    get_engine().get_module<GameSystem>().increment_secret_count();
  }

  void ActivatorHook_Secret::on_activated_start([[maybe_unused]] const ActivatorHookArg& args)
  {
    if (!revealed)
    {
      revealed = true;
      get_engine().get_module<GameSystem>().increment_revealed_count();
    }
  }

}
