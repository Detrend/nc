// Project Nuclidean Source File

#include <engine/core/engine.h>
#include <engine/map/map_dynamics_hooks.h>
#include <engine/map/map_system.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_types.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/entity_type_definitions.h>
#include <engine/ui/user_interface_system.h>
#include <engine/sound/sound_system.h>
#include <engine/sound/sound_resources.h>

#include <engine/game/game_helpers.h>

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

    if (hook_type == "teleport") {
      return std::make_unique<ActivatorHook_Teleport>();
    }

    return nullptr;
  }


  void ActivatorHook_LevelTransition::load(const ActivatorHookLoadArg& arg)
  {
    this->destination = std::string_view(arg.data()["destination"]);
  }

  void ActivatorHook_LevelTransition::on_activated_start([[maybe_unused]] const ActivatorHookArg& args)
  {
    GameSystem& gs = GameSystem::get();
    gs.end_level_and_go_to_another_one_from_gamemode(destination);
  }


  void ActivatorHook_Jumppad::load(const ActivatorHookLoadArg& arg)
  {
    const auto& vec_js = arg.data()["direction"];
    this->direction = vec3(vec_js[0], vec_js[2], vec_js[1]);
  }
  void ActivatorHook_Jumppad::on_activated_start(const ActivatorHookArg& args)
  {
    for (const auto& entity_id : args.entities) {
      Entity *const entity = GameSystem::get().get_entities().get_entity(entity_id);
      (void)entity;
    }
  }

  void ActivatorHook_Secret::load([[maybe_unused]] const ActivatorHookLoadArg& arg)
  {
    get_engine().get_module<GameSystem>().increment_secret_count();
  }

  void ActivatorHook_Secret::on_activated_start([[maybe_unused]] const ActivatorHookArg& args)
  {
    if (!revealed)
    {
      revealed = true;
      get_engine().get_module<GameSystem>().increment_revealed_count();
      get_engine().get_module<UserInterfaceSystem>().get_hud()->show_secret();
      SoundSystem::get().play_oneshot(Sounds::secret, 1.0f);
    }
  }


  void ActivatorHook_Teleport::load([[maybe_unused]] const ActivatorHookLoadArg& arg)
  {
    if (arg.data().contains("is_single_use")) {
      this->is_single_use = arg.data()["is_single_use"];
    }

    this->data.clear();

    for (const auto& destination_tag_js : arg.data()["destinations"]) {
      const unsigned destination_tag = destination_tag_js;
      const auto& destination_js = arg.get_additional_data(destination_tag);
      const unsigned target_tag = destination_js["target"];

      const auto& position_js = destination_js["position"];
      
      const vec3 position(position_js[0], position_js[2], position_js[1]);
      const EntityID target_id = arg.get_entity(target_tag);

      this->data.emplace_back(TeleportData{.entity = target_id, .destination_position = position});
    }
  }

  void ActivatorHook_Teleport::on_activated_start([[maybe_unused]] const ActivatorHookArg& args)
  {
    if (this->is_single_use && this->did_teleport) {
      return;
    }
    this->did_teleport = true;
    for (const auto& it : this->data) {
      GameHelpers::get().request_entity_teleport(it.entity, it.destination_position);
    }
  }

}
