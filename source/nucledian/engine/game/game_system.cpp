// Project Nucledian Source File
#include <common.h>
#include <cvars.h>

#include <engine/game/game_system.h>
#include <engine/player/player.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>

#include <engine/entity/entity_system.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_type_definitions.h>

#include <engine/input/input_system.h>

#include <engine/map/physics.h>
#include <engine/map/map_system.h>
#include <engine/map/map_dynamics.h>

#include <engine/graphics/entities/lights.h>
#include <engine/graphics/entities/sky_box.h>
#include <engine/graphics/resources/texture.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/debug/gizmo.h>

#include <engine/sound/sound_system.h>
#include <engine/sound/sound_resources.h>

#include <game/entity_attachment_manager.h>
#include <game/projectiles.h>

#include <engine/game/game.h>

#include <engine/enemies/enemy.h>
#include <game/projectile.h>
#include <game/weapons.h>
#include <game/item.h>
#include <game/item_resources.h> // PickupTypes::...
#include <game/enemies.h>        // EnemyTypes::...
#include <engine/graphics/entities/prop.h>
#include <engine/map/map_dynamics_hooks.h>

#include <math/lingebra.h>

#ifdef NC_IMGUI
#include <imgui/imgui.h> // for hot reload
#endif

#include <intersect.h>
#include <profiling.h>
#include <stack_vector.h>

#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>
#include <map>
#include <vector>
#include <string>

#include <json/json.hpp>

#include <engine/graphics/entities/lights.h>


//==============================================================================
namespace nc::map_helpers
{ 

//==============================================================================
static void make_sector_helper(
  f32                                         in_floor_y,
  f32                                         in_ceil_y,
  const std::vector<u16>&                     points,
  std::vector<map_building::SectorBuildData>& out,
  int                                         portal_wall_id    = -1,
  WallRelID                                   portal_wall_id_to = 0,
  SectorID                                    portal_sector     = INVALID_SECTOR_ID,
  const SurfaceData                           &floor_surface    = SurfaceData{},
  const SurfaceData                           &ceiling_surface  = SurfaceData{},
  const std::vector<WallSegmentData>          &wall_surfaces    = {}
)
{
  std::vector<map_building::WallBuildData> walls;

  for (int i = 0; i < static_cast<int>(points.size()); ++i)
  {
    auto p = points[i];
    bool is_portal = portal_sector != INVALID_SECTOR_ID && portal_wall_id == i;
    walls.push_back(map_building::WallBuildData
    {
      .point_index            = p,
      .nc_portal_point_index  = is_portal ? portal_wall_id_to : INVALID_WALL_REL_ID,
      .nc_portal_sector_index = is_portal ? portal_sector     : INVALID_SECTOR_ID,
      .surface                = wall_surfaces[i]
    });
  }

  out.push_back(map_building::SectorBuildData
  {
    .points        = std::move(walls),
    .floor_y       = { in_floor_y, in_floor_y },
    .ceil_y        = { in_ceil_y,  in_ceil_y  },
    .floor_surface = floor_surface,
    .ceil_surface  = ceiling_surface,
  });
}

using ActivatorTable = std::vector<ActivatorData>;
using ActivatorMap   = std::map<std::string, ActivatorID>;
using TriggerTable   = std::vector<TriggerData>;

//==============================================================================
template<size_t TSize>
static vec<float, TSize> load_json_vector(const nlohmann::json& js) 
{
  vec<float, TSize> ret;
  for (u32 t = 0; t < TSize; ++t) {
    ret[t] = js[t];
  }
  return ret;
}


//==============================================================================
template<typename T, typename TSupplier>
static void load_json_optional(T& out, const nlohmann::json& js, const cstr key, const TSupplier& supplier) 
{
  if (js.contains(key)) {
    out = supplier(js[key]);
  }
}


//==============================================================================
static bool load_json_flag(const nlohmann::json& js, const cstr key)
{
  if (js.contains(key)) {
    return js[key];
  }
  return false;
}

//==============================================================================
static SurfaceData load_json_surface(const nlohmann::json &js) 
{
  if (const bool should_show = js["show"]) {
    std::string texture_name = js["id"];

    TextureID tid = TextureManager::get()[texture_name].get_texture_id();
    TextureID alt_tid = tid;

    if (js.contains("id_triggered"))
    {
      std::string alt_texture_name = js["id_triggered"];
      alt_tid = TextureManager::get()[alt_texture_name].get_texture_id();
    }

    return SurfaceData
    {
      .texture_id = tid,
      .texture_id_default = tid,
      .texture_id_triggered = alt_tid,
      .scale = js["scale"],
      .rotation = js["rotation"],
      .offset = load_json_vector<2>(js["offset"]),
      .tile_rotations_count = js["tile_rotations_count"],
      .tile_rotation_increment = js["tile_rotation_increment"],
      .should_show = true
    };
  }
  return SurfaceData
  {
    .should_show = false
  };
}

//==============================================================================
static TriggerData load_json_trigger
(
  const nlohmann::json& js, const ActivatorMap& activators
)
{
  TriggerData td;

  std::string activator_name = js["activator"];
  nc_assert(activators.contains(activator_name));

  td.activator = activators.at(activator_name);
  td.timeout          = js["timeout"];
  td.can_turn_off     = load_json_flag(js, "can_turn_off");
  td.player_sensitive = load_json_flag(js, "player_sensitive");
  td.enemy_sensitive  = load_json_flag(js, "enemy_sensitive");
  td.while_alive      = load_json_flag(js, "while_alive");

  return td;
}

//==============================================================================
static WallSegmentData load_json_wall_surface
(
  const nlohmann::json& js,
  const ActivatorMap&   activators,
  TriggerTable&         triggers,
  SectorID              sid,
  WallRelID             wrelid
)
{
  WallSegmentData ret;

  u8 segment_idx = 0;

  for (auto&& js_entry : js)
  {
    auto& entry = ret.surfaces.emplace_back();
    entry.surface = load_json_surface(js_entry);
    entry.end_height = js_entry["end_height"];
    load_json_optional(entry.begin_up_tesselation.xzy, js_entry, "begin_up_direction", load_json_vector<3>);
    load_json_optional(entry.end_up_tesselation.xzy, js_entry, "end_up_direction", load_json_vector<3>);
    load_json_optional(entry.begin_down_tesselation.xzy, js_entry, "begin_down_direction", load_json_vector<3>);
    load_json_optional(entry.end_down_tesselation.xzy, js_entry, "end_down_direction", load_json_vector<3>);

    if (load_json_flag(js_entry, "absolute_directions"))
    {
      entry.flags = static_cast<WallSegmentData::Flags>(entry.flags | WallSegmentData::Flags::absolute_directions);
    }

    if (load_json_flag(js_entry, "flip_side_normals"))
    {
      entry.flags = static_cast<WallSegmentData::Flags>(entry.flags | WallSegmentData::Flags::flip_side_normals);
    }

    if (js_entry.contains("triggers"))
    {
      for (const auto& js_trigger : js_entry["triggers"]) {
        TriggerData td = load_json_trigger(js_trigger, activators);
        td.type = TriggerData::wall;
        td.wall_type.sector = sid;
        td.wall_type.wall = wrelid;
        td.wall_type.segment = segment_idx;

        triggers.push_back(td);
      }
    }

    segment_idx += 1;
  }

  return ret;
}

//==============================================================================
static vec3 load_json_position(const nlohmann::json& js, const cstr &field_name = "position") 
{
  return load_json_vector<3>(js[field_name]).xzy;
}

//==============================================================================
static void load_json_map
(
  const LevelName& level_name,
  MapSectors&      map,
  SectorMapping&   mapping,
  EntityRegistry&  entities,
  MapDynamics&     dynamics,
  EntityID&        player_id
)
{
  using namespace map_building;

  std::ifstream f(get_full_level_path(level_name));
  nc_assert(f.is_open());
  auto data = nlohmann::json::parse(f);

  std::vector<vec2> points;
  std::vector<map_building::SectorBuildData> sectors;

  ActivatorTable activator_table;
  ActivatorMap   activator_map;
  TriggerTable   trigger_table;

  try
  {
    for (auto&& js_activator : data["activators"])
    {
      std::string name = js_activator["name"];
      nc_assert(!activator_map.contains(name));
      activator_map[name] = cast<ActivatorID>(activator_table.size());
      s32 threshold = js_activator["threshold"];
      ActivatorData &ad = activator_table.emplace_back(ActivatorData{cast<u16>(threshold)});
      for (const auto& hook_js : js_activator["hooks"]) {
        std::string hook_type = hook_js["type"];
        auto hook = create_hook_by_type(hook_type);
        hook->load(hook_js);
        ad.hooks.emplace_back(std::move(hook));
      }
    }

    for (auto&& js_point : data["points"])
    {
      points.emplace_back(load_json_vector<2>(js_point));
    }

    SectorID sid = 0;
    for (auto&& js_sector : data["sectors"])
    {
      const f32 floor = js_sector["floor"];
      const f32 ceil = js_sector["ceiling"];

      const SectorID portal_sector = js_sector["portal_target"];
      const int portal_wall = js_sector["portal_wall"];
      const WallRelID portal_destination_wall = js_sector["portal_destination_wall"];

      std::vector<u16> point_indices;
      for (auto&& js_point : js_sector["points"])
      {
        point_indices.emplace_back((u16)(int)js_point);
      }

      auto floor_surface   = load_json_surface(js_sector["floor_surface"]);
      auto ceiling_surface = load_json_surface(js_sector["ceiling_surface"]);

      std::vector<WallSegmentData> wall_surfaces;

      // Surfaces
      WallRelID wrelid = 0;
      for (auto&& js_wall_surface : js_sector["wall_surfaces"])
      {
        WallSegmentData sd = load_json_wall_surface
        (
          js_wall_surface, activator_map, trigger_table, sid, wrelid
        );

        wall_surfaces.push_back(sd);
        wrelid += 1;
      }

      make_sector_helper(floor, ceil, point_indices, sectors, portal_wall, portal_destination_wall, portal_sector, floor_surface, ceiling_surface, wall_surfaces);
      map_building::SectorBuildData& build_data = sectors.back();

      // Multiple states
      if (js_sector.contains("alt_states"))
      {
        for (auto&& js_alt_state : js_sector["alt_states"]) {
          build_data.has_more_states = true;
          build_data.floor_y[1] = js_alt_state["floor"];
          build_data.ceil_y[1] = js_alt_state["ceiling"];
          build_data.move_speed = js_alt_state["move_speed"];

          std::string activator_name = js_alt_state["activator"];
          build_data.activator = activator_map[activator_name];
          nc_assert(activator_map.contains(activator_name));
        }
      }

      // Triggers
      if (js_sector.contains("triggers"))
      {
        for (auto&& js_trigger : js_sector["triggers"])
        {
          TriggerData td = load_json_trigger(js_trigger, activator_map);
          td.type = TriggerData::sector;
          td.sector_type.sector = sid;
          trigger_table.push_back(td);
        }
      }

      sid += 1;
    }
  }
  //catch (nlohmann::json::type_error e)
  //{
  //  nc_crit("{0}", e.what());
  //  nc_assert(false);
  //}
  catch(int){}

  map_building::build_map(points, sectors, map, MapBuildFlag::assert_on_fail);

  // Mapping has to be initialized BEFORE creating any entities!!!
  mapping.on_map_rebuild();

  for (auto&& js_entity : data["entities"])
  {
    const vec3 position = load_json_position(js_entity);
    const vec3 forward = load_json_vector<3>(js_entity["forward"]).xzy;

    if (js_entity["is_player"] == true)
    {
      auto* player = entities.create_entity<Player>(position, forward);
      player_id = player->get_id();
    }
    else
    {
      const EnemyTypes::evalue entity_type = js_entity["entity_type"];
      Entity* enemy = entities.create_entity<Enemy>(position, forward, entity_type);

      if (js_entity.contains("triggers"))
      {
        for (auto&& js_trigger : js_entity["triggers"])
        {
          TriggerData td = load_json_trigger(js_trigger, activator_map);
          td.type = TriggerData::entity;
          td.entity_type.entity = enemy->get_id();
          trigger_table.push_back(td);
        }
      }
    }
  }

  for (auto&& js_pickup : data["pickups"])
  {
    const vec3 position = load_json_position(js_pickup);
    const PickupTypes::evalue pickup_type = static_cast<PickupTypes::evalue>(js_pickup["type"]);

    Entity* pickup = entities.create_entity<PickUp>(position, pickup_type);

    if (js_pickup.contains("triggers"))
    {
      for (auto&& js_trigger : js_pickup["triggers"])
      {
        TriggerData td = load_json_trigger(js_trigger, activator_map);
        td.type = TriggerData::entity;
        td.entity_type.entity = pickup->get_id();
        trigger_table.push_back(td);
      }
    }
  }

  for (auto&& js_prop : data["props"])
  {
    const vec3 position = load_json_position(js_prop);
    const f32 radius = js_prop["radius"];
    const f32 height = js_prop["height"];
    const Appearance appearance {
      js_prop["sprite"],
      load_json_vector<3>(js_prop["direction"]).xzy,
      js_prop["scale"],
      static_cast<Appearance::SpriteMode>(js_prop["mode"]),
      static_cast<Appearance::PivotMode>(js_prop["pivot"]),
      static_cast<Appearance::ScalingMode>(js_prop["scaling"]),
      static_cast<Appearance::RotationMode>(js_prop["rotation"]),
    };

    entities.create_entity<Prop>(position, radius, height, appearance);
  }
  for (auto&& js_light : data["directional_lights"])
  {
    //const vec3 position = load_json_position(js_light);
    const color4 color = load_json_vector<4>(js_light["color"]);
    const vec3 direction = load_json_vector<3>(js_light["direction"]);
    const float intensity = js_light["intensity"];

    entities.create_entity<DirectionalLight>(direction, intensity, color);
  }
  for (auto&& js_light : data["point_lights"])
  {
    const vec3   position  = load_json_position(js_light);
    const color4 color     = load_json_vector<4>(js_light["color"]);
    const float  intensity = js_light["intensity"];
    const float  radius    = js_light.contains("radius") 
      ? cast<f32>(js_light["radius"])
      : 3.0f;
    const float  falloff   = js_light.contains("falloff")
      ? cast<f32>(js_light["falloff"])
      : 1.0f;

    entities.create_entity<PointLight>(position, radius, intensity, falloff, color);
  }
  for (auto&& js_light : data["ambient_lights"])
  {
    const float intensity = js_light["intensity"];

    entities.create_entity<AmbientLight>(intensity);
  }
  for (auto&& js_skybox : data["skyboxes"])
  {
    const std::string texture = js_skybox["texture"];
    const float exposure = js_skybox["exposure"];
    const bool use_gamma = js_skybox["use_gamma_correction"];
    const GLuint sky_box_map = TextureManager::get().get_equirectangular_map(texture, ResLifetime::Game);
    entities.create_entity<SkyBox>(sky_box_map, exposure, use_gamma);
  }

  dynamics.activators = std::move(activator_table);
  dynamics.triggers   = std::move(trigger_table);

  dynamics.on_map_rebuild_and_entities_created();
}


}

namespace nc
{

#if NC_HOT_RELOAD
struct HotReloadData
{
  inline static vec3 player_position = VEC3_ZERO;
  inline static f32  player_yaw      = 0.0f;
  inline static f32  player_pitch    = 0.0f;
  inline static bool has_data        = false;
};
#endif

constexpr cstr SAVE_DIR_RELATIVE = "save";
constexpr cstr DEMO_DIR_RELATIVE = "demo";
constexpr cstr SAVE_FILE_SUFFIX  = ".ncs";
constexpr cstr DEMO_FILE_SUFFIX  = ".demo";

//==============================================================================
EngineModuleId GameSystem::get_module_id()
{
  return EngineModule::entity_system;
}

//==============================================================================
GameSystem& GameSystem::get()
{
  return get_engine().get_module<GameSystem>();
}

//==============================================================================
GameSystem::GameSystem()  = default;
GameSystem::~GameSystem() = default;

//==============================================================================
bool GameSystem::init()
{
  this->cleanup_map();
  return true;
}

//==============================================================================
void GameSystem::post_init()
{
  // Load game saves and populate the database
  namespace fs = std::filesystem;

  for (const auto& entry : fs::directory_iterator(SAVE_DIR_RELATIVE))
  {
    if (!entry.is_regular_file())
    {
      // Ignore
      continue;
    }

    if (entry.path().extension() != SAVE_FILE_SUFFIX)
    {
      // Not a savefile
      continue;
    }

    std::ifstream input_file;
    input_file.open(entry, std::ios::binary | std::ios::ate);
    if (!input_file.is_open())
    {
      // What?
      nc_warn("Failed to open save game file \"{}\", skipping..", entry.path().string());
      continue;
    }

    auto size = input_file.tellg();
    input_file.seekg(0, std::ios::beg);

    std::vector<byte> data(size, 0);
    input_file.read(reinterpret_cast<char*>(&data[0]), size);

    SaveGameData save_game;
    if (!deserialize_save_game_from_bytes(save_game, data))
    {
      // Try to deserialize the data
      nc_warn("Failed to read save game data from file \"{}\"", entry.path().string());
      continue;
    }

    // Insert into the database
    this->get_save_game_db().push_back(SaveDbEntry
    {
      .data  = save_game,
      .dirty = false,
    });

    this->last_save_id = std::max(this->last_save_id, save_game.id);
  }

  // Schedule the loading of the first level..
  // This is probably only temporary.
  this->request_level_change(Levels::TEST_LEVEL);
}

//==============================================================================
void GameSystem::pre_terminate()
{
  // Store the dirty save games
  for (const auto&[save, dirty] : this->get_save_game_db())
  {
    if (!dirty)
    {
      continue;
    }

    std::vector<byte> bytes;
    serialize_save_game_to_bytes(save, bytes);

    std::string filename = std::format
    (
      "{}/save{}.{}", SAVE_DIR_RELATIVE, save.id, SAVE_FILE_SUFFIX
    );

    std::ofstream output;
    output.open(filename, std::ios::binary | std::ios::trunc);
    if (!output.is_open())
    {
      nc_crit("Could not open savegame file \"{}\" for writing.", filename);
      continue;
    }

    output.write(reinterpret_cast<char*>(&bytes[0]), bytes.size());
    output.flush();
    output.close();
  }
}

//==============================================================================
void GameSystem::game_update(f32 delta)
{
  NC_SCOPE_PROFILER(GameSystemUpdate)

#ifdef NC_DEBUG_DRAW
  this->handle_raycast_debug();
#endif

#ifdef NC_EDITOR
  this->handle_hot_reload();
#endif

  u64 num_frames_to_simulate = 1;

  if (journal.state == JournalState::playing)
  {
    // Calculate proper number of frames to simulate to keep a pace
    f32 dt_left = delta;
    num_frames_to_simulate = 0;
    u64 idx = journal.rover;
    while (dt_left > 0.0f && idx < journal.frames.size())
    {
      num_frames_to_simulate += 1;
      dt_left -= journal.frames[idx].delta;
      idx += 1;
    }

#if defined(NC_DEBUG_DRAW) && defined(NC_IMGUI)
    if (ImGui::IsKeyPressed(ImGuiKey_Space))
    {
      journal.paused = !journal.paused;
    }

    u64 frames_left = journal.frames.size() - journal.rover - 1;

    if (journal.paused)
    {
      num_frames_to_simulate = 0;

      bool lshift = ImGui::IsKeyDown(ImGuiKey_LeftShift);
      bool rshift = ImGui::IsKeyDown(ImGuiKey_RightShift);
      bool shift  = lshift | rshift;
      bool skip   = ImGui::IsKeyPressed(ImGuiKey_RightArrow);
      if (skip)
      {
        num_frames_to_simulate = min(shift ? 100_u64 : 1_u64, frames_left);
      }
    }

    if (journal.skip_to != -1)
    {
      nc_assert(journal.skip_to >= journal.rover);
      u64 to_skip = cast<u64>(journal.skip_to - journal.rover);
      num_frames_to_simulate = min(to_skip, frames_left);
      journal.skip_to = -1;
    }
#endif

    if (journal.rover == journal.frames.size() - 1)
    {
      // Journal ended
      num_frames_to_simulate = 0;
    }
  }

  while (num_frames_to_simulate-->0)
  {
    GameInputs curr_input = InputSystem::get().get_inputs();
    GameInputs prev_input = InputSystem::get().get_prev_inputs();

    PlayerSpecificInputs player_input_curr, player_input_prev;
    f32 delta_time = 0.0f;

    if (journal.state == JournalState::playing)
    {
      PlayerSpecificInputs empty_inputs;
      std::memset(&empty_inputs, 0, sizeof(empty_inputs));

      nc_assert(journal.rover < journal.frames.size());
      const JournalFrame& frame = journal.frames[journal.rover];

      player_input_prev = journal.rover
        ? journal.frames[journal.rover-1].inputs
        : empty_inputs;

      player_input_curr = frame.inputs;
      delta_time        = frame.delta;

#if DO_JOURNAL_CHECKS
      Player* player = this->get_player();
      nc_assert(player->get_position() == frame.player_position);
      this->get_entities().for_each(EntityTypes::all, [&](Entity& entity)
      {
        nc_assert(frame.alive_entities.contains(entity.get_id().as_u64()));
      });
#endif

      journal.rover += 1;
    }
    else
    {
      player_input_curr = curr_input.player_inputs;
      player_input_prev = prev_input.player_inputs;
      delta_time        = delta;

      if (journal.state == JournalState::recording)
      {
        // Record the inputs as well
        journal.frames.push_back(JournalFrame
        {
          .inputs = player_input_curr,
          .delta  = delta_time,
        });

#if DO_JOURNAL_CHECKS
      Player* player = this->get_player();
      journal.frames.back().player_position = player->get_position();
      this->get_entities().for_each(EntityTypes::all, [&](Entity& entity)
      {
        journal.frames.back().alive_entities.insert(entity.get_id().as_u64());
      });
#endif
      }
    }

    game->update
    (
      delta_time, player_input_curr, player_input_prev
    );
  }

#ifdef NC_IMGUI
  this->handle_demo_debug();
#endif
}

//==============================================================================
#ifdef NC_EDITOR
void GameSystem::handle_hot_reload()
{
  if (ImGui::IsKeyReleased(ImGuiKey_F5))
  {
    if (Player* player = get_player())
    {
      player->hot_reload_get_pos_rot
      (
        HotReloadData::player_position,
        HotReloadData::player_yaw,
        HotReloadData::player_pitch
      );
      HotReloadData::has_data = true;
      this->request_level_change(this->get_level_name());
    }
  }
  else if (ImGui::IsKeyReleased(ImGuiKey_F7))
  {
    if (Player* player = get_player())
    {
      HotReloadData::has_data = false;
      this->request_level_change(this->get_level_name());
    }
  }
}
#endif

//==============================================================================
#if NC_HOT_RELOAD
static void notify_hot_reload_post_map_build()
{
  if (HotReloadData::has_data)
  {
    if (Player* player = GameSystem::get().get_player())
    {
      player->hot_reload_set_pos_rot
      (
        HotReloadData::player_position,
        HotReloadData::player_yaw,
        HotReloadData::player_pitch
      );
    }

    HotReloadData::player_position = VEC3_ZERO;
    HotReloadData::player_yaw      = 0.0f;
    HotReloadData::player_pitch    = 0.0f;
    HotReloadData::has_data        = false;
  }
}
#endif

//==============================================================================
void GameSystem::on_event(ModuleEvent& event)
{
  switch (event.type)
  {
    case ModuleEventType::post_init:
    {
      this->post_init();
    }
    break;

    case ModuleEventType::pre_terminate:
    {
      this->pre_terminate();
    }
    break;

    case ModuleEventType::frame_start:
    {
      // MR says: We want this to happen at the frame start and
      //          not on cleanup, because for the first frame of
      //          the game there would be no level or entities.
      //          This simplifies the stuff a lot.
      if (this->scheduled_level_id != INVALID_LEVEL_NAME)
      {
        get_engine().send_event
        (
          ModuleEvent{.type = ModuleEventType::before_map_rebuild}
        );

        // do level transition
        this->cleanup_map();
        this->build_map(this->scheduled_level_id);
        this->level_name = this->scheduled_level_id;
        this->scheduled_level_id = INVALID_LEVEL_NAME;

        get_engine().send_event
        (
          ModuleEvent{.type = ModuleEventType::after_map_rebuild}
        );

#if NC_HOT_RELOAD
        notify_hot_reload_post_map_build();
#endif
      }
    }
    break;

    case ModuleEventType::game_update:
    {
      this->game_update(event.update.dt);
    }
    break;
  }
}

//==========================================================
Player* GameSystem::get_player()
{
  return this->get_entities().get_entity<Player>(game->player_id);
}

//==============================================================================
EntityRegistry& GameSystem::get_entities()
{
  nc_assert(game->entities);
  return *game->entities;
}

//==============================================================================
MapDynamics& GameSystem::get_map_dynamics()
{
  nc_assert(game->dynamics);
  return *game->dynamics;
}

//==============================================================================
const EntityRegistry& GameSystem::get_entities() const
{
  return const_cast<GameSystem*>(this)->get_entities();
}

//==============================================================================
SaveGameData GameSystem::save_game() const
{
  SaveGameData save;
  save.last_level = this->get_level_name();
  save.time       = SaveGameData::Clock::now();
  save.id         = ++last_save_id;
  return save;
}

//==============================================================================
void GameSystem::load_game(const SaveGameData& save)
{
  this->request_level_change(save.last_level);
}

//==============================================================================
GameSystem::SaveDatabase& GameSystem::get_save_game_db()
{
  return save_db;
}

//==============================================================================
LevelName GameSystem::get_level_name() const
{
  return level_name;
}

//==============================================================================
void GameSystem::request_level_change(LevelName new_level)
{
  // Another level already scheduled.
  nc_assert(this->scheduled_level_id == INVALID_LEVEL_NAME);

  this->scheduled_level_id = new_level;
}

//==============================================================================
const MapSectors& GameSystem::get_map() const
{
  nc_assert(game->map);
  return *game->map;
}

//==============================================================================
const SectorMapping& GameSystem::get_sector_mapping() const
{
  nc_assert(game->mapping);
  return *game->mapping;
}

//==============================================================================
PhysLevel GameSystem::get_level() const
{
  return PhysLevel
  {
    .entities = this->get_entities(),
    .map      = this->get_map(),
    .mapping  = this->get_sector_mapping(),
  };
}

//==============================================================================
Projectile* GameSystem::spawn_projectile
(
  ProjectileType type, vec3 from, vec3 dir, EntityID author
)
{
  // Spawn projectile
  Projectile* projectile = get_entities().create_entity<Projectile>
  (
    from, dir, author, type 
  );

  if (vec3 light_col = PROJECTILE_STATS[type].light_color; light_col != VEC3_ZERO)
  {
    // And its light
    PointLight* light = get_entities().create_entity<PointLight>
    (
      from, 3.0f, 2.5f, 1.15f, light_col
    );

    // And attach it
    get_attachment_mgr().attach_entity
    (
      light->get_id(), projectile->get_id(), EntityAttachmentFlags::all
    );
  }

  return projectile;
}

//==============================================================================
void GameSystem::play_3d_sound
(
  vec3 position, SoundID sound, f32 distance, f32 volume
)
{
  Player* player = this->get_player();
  if (!player)
  {
    return;
  }

  nc_assert(player->get_camera());

  f32 dist_vol = this->get_level().calc_3d_sound_volume
  (
    player->get_camera()->get_position(),
    position,
    distance
  );

  if (dist_vol > 0.0f)
  {
    SoundSystem::get().play(sound, volume * dist_vol);
  }
}

//==============================================================================
void GameSystem::on_player_traversed_nc_portal(EntityID player, mat4 transform)
{
  get_entities().for_each<Enemy>([&](Enemy& enemy)
  {
    enemy.on_player_traversed_nc_portal(player, transform);
  });
}

//==============================================================================
EntityAttachment& GameSystem::get_attachment_mgr()
{
  nc_assert(game->attachment);
  return *game->attachment;
}

//==============================================================================
const EntityAttachment& GameSystem::get_attachment_mgr() const
{
  return const_cast<GameSystem*>(this)->get_attachment_mgr();
}

//==============================================================================
u64 GameSystem::get_frame_idx() const
{
  return game->frame_idx;
}

//==============================================================================
void GameSystem::cleanup_map()
{
  game.reset();
  game = std::make_unique<Game>();
}

//==============================================================================
// Checks if the entity should be snapped to floor or ceiling. Returns the
// snapped y-position if so.
static bool recalc_snap_entity_height
(
  Entity&              entity,
  const MapSectors&    map,
  const SectorMapping& mapping,
  f32&                 out_height
)
{
  SectorSnapType snap = entity.get_snap_type();
  if (snap == SectorSnapTypes::free)
  {
    return false;
  }

  EntityID entity_id = entity.get_id();
  bool     to_floor  = snap == SectorSnapTypes::floor;
  f32      default_h = to_floor ? -FLT_MAX : FLT_MAX;
  f32      best_h    = default_h;

  mapping.for_each_sector_of_entity(entity_id, [&](SectorID sid, mat4 t)
  {
    // Avoid matrix multiplications and inversions
    f32 offset = entity.get_snap_offset();

    const SectorData& sd = map.sectors[sid];
    const f32 heights[2] = {sd.ceil_height - offset, sd.floor_height + offset};

    f32 height   = heights[to_floor];
    f32 t_height = t[1].y * height + t[3].y;
    f32 real_h   = height - (t_height - height);

    const bool conditions[2] = {real_h < best_h, real_h > best_h};

    // Search for 
    if (conditions[to_floor])
    {
      best_h = real_h;
    }
  });

  out_height = best_h;
  return best_h != default_h;
}

//==============================================================================
void GameSystem::build_map(LevelName level)
{
  nc_assert(!game->map && !game->mapping && !game->entities);

  game->map        = std::make_unique<MapSectors>();
  game->mapping    = std::make_unique<SectorMapping>(*game->map);
  game->entities   = std::make_unique<EntityRegistry>();
  game->dynamics   = std::make_unique<MapDynamics>
  (
    *game->map, *game->entities, *game->mapping
  );
  game->attachment = std::make_unique<EntityAttachment>(*game->entities);

  game->dynamics->sector_change_callback = [](SectorID sector)
  {
    // Rebuild the sector geometry
    GraphicsSystem::get().mark_sector_dirty(sector);

    const MapSectors&     map      = GameSystem::get().get_map();
    const SectorMapping&  mapping  = GameSystem::get().get_sector_mapping();

    EntityRegistry& registry = GameSystem::get().get_entities();

    // Move pickups with the floor
    struct NewHeight
    {
      Entity* entity;
      f32     height;
    };
    StackVector<NewHeight, 16> new_heights;

    mapping.for_each_in_sector(sector, [&](EntityID entity_id, mat4)
    {
      auto it = std::find_if(new_heights.begin(), new_heights.end(), [&](auto& nh)
      {
        return nh.entity->get_id() == entity_id;
      });

      if (it != new_heights.end())
      {
        // Already iterated this entity from a different portal
        return;
      }

      Entity* entity = registry.get_entity(entity_id);
      nc_assert(entity);

      if (f32 out_h; recalc_snap_entity_height(*entity, map, mapping, out_h))
      {
        if (out_h != entity->get_position().y)
        {
          new_heights.push_back(NewHeight{entity, out_h});
        }
      }
    });

    // We have to change the heights here separately because doing it at the
    // same time would cause a rebuild of the mapping WHILE iterating it.
    for (NewHeight to_change : new_heights)
    {
      vec3 pos = to_change.entity->get_position();
      to_change.entity->set_position(with_y(pos, to_change.height));
    }
  };

  game->entities->add_listener(game->mapping.get());
  game->entities->add_listener(game->attachment.get());

  map_helpers::load_json_map
  (
    level,
    *game->map,
    *game->mapping,
    *game->entities,
    *game->dynamics,
    game->player_id
  );

  // Now snap all required entities to the floor
  game->entities->for_each(EntityTypes::all, [&](Entity& entity)
  {
    if (f32 out_h; recalc_snap_entity_height(entity, *game->map, *game->mapping, out_h))
    {
      if (vec3 pos = entity.get_position(); pos.y != out_h)
      {
        entity.set_position(with_y(pos, out_h));
      }
    }
  });
}

//==============================================================================
void GameSystem::Journal::reset(GameSystem::JournalState to_state)
{
  state = to_state;
  paused = false;
  rover = 0;
}

//==============================================================================
void GameSystem::Journal::reset_and_clear(GameSystem::JournalState to_state)
{
  reset(to_state);
  frames.clear();
}

//==============================================================================
#ifdef NC_DEBUG_DRAW
void GameSystem::handle_raycast_debug()
{
  Player* player = this->get_player();

  if (player && player->get_camera() && CVars::debug_player_raycasts)
  {
    if (ImGui::Begin("Raycast debug", &CVars::debug_player_raycasts))
    {
      const Camera* camera = player->get_camera();
      const auto& lvl = GameSystem::get().get_level();

      static f32 ray_len = 10.0f;
      ImGui::SliderFloat("Ray lenght", &ray_len, 1.0f, 25.0f);
	
      const vec3 eye_pos  = camera->get_position();
      const vec3 look_dir = camera->get_forward();
	
      const vec3 ray_start = eye_pos;
      const vec3 ray_end   = eye_pos + look_dir * ray_len;

      PhysLevel::Portals portals;

      constexpr auto TYPES_TO_CHECK = std::array
      {
        EntityTypes::enemy, EntityTypes::pickup, EntityTypes::prop
      };

      ImGui::Separator();

      ImGui::Text("Entity types to hit:");

      static EntityTypeMask hit_types = 0;
      for (EntityType entity_type : TYPES_TO_CHECK)
      {
        bool value = hit_types & entity_type_to_mask(entity_type);

        if (ImGui::Checkbox(ENTITY_TYPE_NAMES[entity_type], &value))
        {
          if (value)
          {
            hit_types |= entity_type_to_mask(entity_type);
          }
          else
          {
            hit_types &= ~entity_type_to_mask(entity_type);
          }
        }
      }
	
      ImGui::Separator();

      CollisionHit hit = lvl.cylinder_cast_3d
      (
        ray_start, ray_end, 0.25f, 0.25f, hit_types, &portals
      );

      if (hit)
      {
        vec3 hit_point  = ray_start + (ray_end - ray_start) * hit.coeff;
        vec3 out_normal = hit.normal;
      
        for (const auto&[wid, sid] : portals)
        {
          mat4 transform = lvl.map.calc_portal_to_portal_projection(sid, wid);
          hit_point  = transform * vec4{hit_point,  1.0f};
          out_normal = transform * vec4{out_normal, 0.0f};
        }

        Gizmo::create_line(1.0f, hit_point, hit_point + out_normal, colors::RED);

        vec3 pt = ray_start + (ray_end - ray_start) * hit.coeff;

        ImGui::Text("Hit distance: %.3f", ray_len * hit.coeff);
        ImGui::Text("Hit pt: [%.2f, %.2f, %.2f]", pt.x, pt.y, pt.z);
        ImGui::Text("Num portals: %d", cast<int>(portals.size()));

        ImGui::Separator();

        if (hit.is_wall_hit())
        {
          SectorID  sid    = hit.hit.sector.sector_id;
          WallID    wid    = hit.hit.sector.wall_id;
          int       wrelid = wid - lvl.map.sectors[sid].int_data.first_wall;
          ImGui::Text("Hit type: wall");
          ImGui::Text("Sector:   %d", cast<int>(sid));
          ImGui::Text("Wall:     %d", cast<int>(wid));
          ImGui::Text("Rel wall: %d", wrelid);
          ImGui::Text("Segment:  %d", cast<int>(hit.hit.sector.wall_segment_id));
        }
        else if (hit.is_floor_hit() || hit.is_ceil_hit())
        {
          ImGui::Text("Hit type: floor/ceil");
          ImGui::Text("SectorID: %d", cast<int>(hit.hit.sector.sector_id));
        }
        else if (hit.is_entity_hit())
        {
          EntityID id = hit.hit.entity.entity_id;

          std::string id_as_str = std::format
          (
            "[{}]:[{}]", ENTITY_TYPE_NAMES[id.type], id.idx
          );

          ImGui::Text("Hit type: entity");
          ImGui::Text("Entity:   %s", id_as_str.c_str());
        }
      }
      else
      {
        ImGui::Text("No hit");
      }
      ImGui::End();
    }
  }
}

//==============================================================================
/*static*/ std::vector<std::string> GameSystem::list_available_demos()
{
  namespace fs = std::filesystem;

  std::vector<std::string> result;
  fs::path demo_dir = DEMO_DIR_RELATIVE;

  if (!fs::exists(demo_dir) || !fs::is_directory(demo_dir))
  {
    return result;
  }

  for (auto &entry : fs::directory_iterator(demo_dir))
  {
    if (entry.is_regular_file())
    {
      auto path = entry.path();
      if (path.extension() == DEMO_FILE_SUFFIX)
      {
        result.push_back(path.filename().string());
      }
    }
  }

  return result;
}

//==============================================================================
/*static*/ void GameSystem::save_demo_data
(
  const std::string& lvl_name,
  u8*                data,
  u64                data_size
)
{
  namespace fs = std::filesystem;

#if DO_JOURNAL_CHECKS
  nc_assert(false, "You are trying to save sanitization data into a demo file");
#endif

  fs::path demo_dir = DEMO_DIR_RELATIVE;
  if (!fs::exists(demo_dir))
  {
    fs::create_directory(demo_dir);
  }

  int index = 1;
  std::string file_name;
  fs::path full_path;

  while (true)
  {
    file_name = lvl_name + "_" + std::to_string(index) + DEMO_FILE_SUFFIX;
    full_path = demo_dir / file_name;

    if (!fs::exists(full_path))
    {
      break;
    }

    ++index;
  }

  std::ofstream out(full_path, std::ios::binary);
  nc_assert(out);

  out.write(recast<cstr>(data), cast<std::streamsize>(data_size));
}

//==============================================================================
/*static*/ void GameSystem::load_demo_from_bytes
(
  const std::string& demo_name,
  std::vector<u8>&   out
)
{
  namespace fs = std::filesystem;

#if DO_JOURNAL_CHECKS
  nc_assert(false, "You are trying to load sanitization data into a demo file");
#endif

  fs::path demo_dir = DEMO_DIR_RELATIVE;
  if (!fs::exists(demo_dir) || !fs::is_directory(demo_dir))
  {
    nc_assert(false);
    return;
  }

  fs::path full_path = demo_dir / demo_name;
  if (!fs::exists(full_path) || !fs::is_regular_file(full_path))
  {
    nc_assert(false);
    return;
  }

  std::ifstream in(full_path, std::ios::binary);
  nc_assert(in);

  in.seekg(0, std::ios::end);
  std::streamsize size = in.tellg();
  in.seekg(0, std::ios::beg);

  out.resize(size);

  if (!in.read(recast<char*>(out.data()), size))
  {
    nc_assert(false);
    return;
  }
}

//==============================================================================
void GameSystem::handle_demo_debug()
{
  if (ImGui::IsKeyReleased(ImGuiKey_F6))
  {
    CVars::debug_demo_recording = !CVars::debug_demo_recording;
  }

  if (!CVars::debug_demo_recording)
  {
    return;
  }

  if (ImGui::Begin("Journal Debug", &CVars::debug_demo_recording))
  {
    constexpr cstr STATE_NAMES[] {"recording", "playing", "none"};
    cstr journal_state = STATE_NAMES[cast<u8>(journal.state)];
    ImGui::Text("Journal state: %s", journal_state);

    if (journal.state == JournalState::recording)
    {
      ImGui::Text("Frames recorded: %d", cast<int>(journal.frames.size()));
      if (ImGui::Button("Play"))
      {
        this->request_level_change(this->get_level_name());
        journal.reset(JournalState::playing);
      }
    }
    else if (journal.state == JournalState::playing)
    {
      ImGui::Text("Currend idx: %d", cast<int>(journal.rover)-1);
      if (ImGui::Button("Save to filesystem"))
      {
        save_demo_data
        (
          level_name,
          recast<u8*>(journal.frames.data()),
          journal.frames.size() * sizeof(JournalFrame)
        );
      }

      static int skip_to_idx = 0;
      int from = cast<int>(journal.rover + 1);
      int to   = cast<int>(journal.frames.size() - 1);
      ImGui::SliderInt("Skip idx", &skip_to_idx, from, to);
      ImGui::SameLine();
      if (ImGui::Button("Skip"))
      {
        journal.skip_to = skip_to_idx;
      }
    }

    if (ImGui::Button("Start new demo"))
    {
      this->request_level_change(this->get_level_name());
      journal.reset_and_clear(JournalState::recording);
    }

    if (ImGui::Button("Start new game without recording"))
    {
      this->request_level_change(this->get_level_name());
      journal.reset_and_clear(JournalState::none);
    }

    ImGui::Separator();

    ImGui::Text("Open demo:");
    std::vector<std::string> available_demos = list_available_demos();
    for (const std::string& demo : available_demos)
    {
      if (ImGui::Button(demo.c_str()))
      {
        this->request_level_change(this->get_level_name());
        std::vector<u8> bytes;
        load_demo_from_bytes(demo, bytes);
        u64 num_frames = bytes.size() / sizeof(JournalFrame);

        journal.reset(JournalState::playing);
        journal.frames.resize(num_frames);
        std::memcpy(journal.frames.data(), bytes.data(), bytes.size());
      }
    }
  }
  ImGui::End();
}

#endif

}
