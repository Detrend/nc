// Project Nucledian Source File
#include <engine/player/thing_system.h>
#include <engine/player/player.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>

#include <engine/map/map_system.h>
#include <engine/entity/entity_system.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_type_definitions.h>

#include <engine/input/input_system.h>
#include <engine/map/physics.h>

#include <engine/graphics/resources/texture.h>
#include <engine/graphics/graphics_system.h>
#include <engine/graphics/gizmo.h>

#include <engine/sound/sound_system.h>
#include <engine/sound/sound_resources.h>

#include <game/entity_attachment_manager.h>

#include <engine/enemies/enemy.h>
#include <game/projectile.h>
#include <game/weapons.h>
#include <game/item.h>
#include <game/item_resources.h> // PickupTypes::...
#include <game/enemies.h>        // EnemyTypes::...

#include <common.h>
#include <cvars.h>

#include <intersect.h>
#include <profiling.h>

#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>

#include <json/json.hpp>

#include <engine/graphics/lights.h>


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
  const std::vector<WallSurfaceData>              &wall_surfaces    = {}
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
    .floor_y       = in_floor_y,
    .ceil_y        = in_ceil_y,
    .floor_surface = floor_surface,
    .ceil_surface  = ceiling_surface,
  });
}



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
    return SurfaceData
    {
      .texture_id = TextureManager::get()[texture_name].get_texture_id(),
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
static WallSurfaceData load_json_wall_surface(const nlohmann::json& js) 
{
  WallSurfaceData ret;

  for (auto&& js_entry : js) {
    auto &entry = ret.surfaces.emplace_back();
    entry.surface = load_json_surface(js_entry);
    entry.end_height = js_entry["end_height"];
    load_json_optional(entry.begin_up_tesselation.xzy,  js_entry, "begin_up_direction", load_json_vector<3>);
    load_json_optional(entry.end_up_tesselation.xzy,    js_entry, "end_up_direction", load_json_vector<3>);
    load_json_optional(entry.begin_down_tesselation.xzy,js_entry, "begin_down_direction", load_json_vector<3>);
    load_json_optional(entry.end_down_tesselation.xzy,  js_entry, "end_down_direction", load_json_vector<3>);
    if (load_json_flag(js_entry, "absolute_directions")) {
      entry.flags = static_cast<WallSurfaceData::Flags>(entry.flags | WallSurfaceData::Flags::absolute_directions);
    }
    if (load_json_flag(js_entry, "flip_side_normals")) {
      entry.flags = static_cast<WallSurfaceData::Flags>(entry.flags | WallSurfaceData::Flags::flip_side_normals);
    }
  }

  return ret;
}

//==============================================================================
static vec3 load_json_position(const nlohmann::json& js, const cstr &field_name = "position") 
{
  return load_json_vector<3>(js[field_name]).xzy;
}

//==============================================================================
static void load_json_map(const LevelName &level_name, MapSectors& map, SectorMapping& mapping, EntityRegistry& entities, EntityID& player_id)
{
  std::ifstream f(get_full_level_path(level_name));
  nc_assert(f.is_open());
  auto data = nlohmann::json::parse(f);

  std::vector<vec2> points;
  std::vector<map_building::SectorBuildData> sectors;

  try {
    for (auto&& js_point : data["points"]) {
      points.emplace_back(load_json_vector<2>(js_point));
    }
    for (auto&& js_sector : data["sectors"]) {
      const float floor = js_sector["floor"];
      const float ceil = js_sector["ceiling"];
      const SectorID portal_sector = js_sector["portal_target"];
      const int portal_wall = js_sector["portal_wall"];
      const WallRelID portal_destination_wall = js_sector["portal_destination_wall"];

      std::vector<u16> point_indices;
      for (auto&& js_point : js_sector["points"]) {
        point_indices.emplace_back((u16)(int)js_point);
      }

      auto floor_surface = load_json_surface(js_sector["floor_surface"]);
      auto ceiling_surface = load_json_surface(js_sector["ceiling_surface"]);
      std::vector<WallSurfaceData> wall_surfaces;
      for (auto&& js_wall_surface : js_sector["wall_surfaces"]) {
        wall_surfaces.emplace_back(load_json_wall_surface(js_wall_surface));
      }

      make_sector_helper(floor, ceil, point_indices, sectors, portal_wall, portal_destination_wall, portal_sector, floor_surface, ceiling_surface, wall_surfaces);
    }
  }
  catch (nlohmann::json::type_error e) {
    nc_crit("{0}", e.what());
  }

  using namespace map_building::MapBuildFlag;

  map_building::build_map(
    points, sectors, map,
    //omit_convexity_clockwise_check |
    //omit_sector_overlap_check      |
    //omit_wall_overlap_check        |
    assert_on_fail
  );

  mapping.on_map_rebuild();


  for (auto&& js_entity : data["entities"]) {
    const vec3 position = load_json_position(js_entity);
    const vec3 forward = load_json_vector<3>(js_entity["forward"]).xzy;

    if (js_entity["is_player"] == true) {
      auto* player = entities.create_entity<Player>(position, forward);
      player_id = player->get_id();
    }
    else {
      // Beware that these fuckers can shoot you even if you do not see them and therefore kill you during the normal level testing.
      const EnemyTypes::evalue entity_type = js_entity["entity_type"];
      entities.create_entity<Enemy>(position, forward, entity_type);
    }
  }

  for (auto&& js_pickup : data["pickups"]) {
    const vec3 position = load_json_position(js_pickup);
    const PickupTypes::evalue pickup_type = static_cast<PickupTypes::evalue>(js_pickup["type"]);
    entities.create_entity<PickUp>(position, pickup_type);
  }
  for (auto&& js_light : data["directional_lights"]) {
    //const vec3 position = load_json_position(js_light);
    const color4 color = load_json_vector<4>(js_light["color"]);
    const vec3 direction = load_json_vector<3>(js_light["direction"]);
    const float intensity = js_light["intensity"];

    entities.create_entity<DirectionalLight>(direction, intensity, color);
  }
  for (auto&& js_light : data["point_lights"]) {
    const vec3 position = load_json_position(js_light);
    const color4 color = load_json_vector<4>(js_light["color"]);
    const float intensity = js_light["intensity"];

    const float constant = js_light["constant"];
    const float linear = js_light["linear"];
    const float quadratic = js_light["quadratic"];

    entities.create_entity<PointLight>(position, intensity, constant, linear, quadratic, color);
  }
  for (auto&& js_light : data["ambient_lights"]) {
    const float intensity = js_light["intensity"];

    entities.create_entity<AmbientLight>(intensity);
  }

}


}

namespace nc
{

constexpr cstr SAVE_DIR_RELATIVE = "save";
constexpr cstr SAVE_FILE_SUFFIX  = ".ncs";

//==============================================================================
EngineModuleId ThingSystem::get_module_id()
{
  return EngineModule::entity_system;
}

//==============================================================================
ThingSystem& ThingSystem::get()
{
  return get_engine().get_module<ThingSystem>();
}

//==============================================================================
ThingSystem::ThingSystem()  = default;
ThingSystem::~ThingSystem() = default;

//==============================================================================
bool ThingSystem::init()
{
  return true;
}

//==============================================================================
void ThingSystem::post_init()
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
  this->request_level_change(Levels::LEVEL_1);
}

//==============================================================================
void ThingSystem::pre_terminate()
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
void ThingSystem::on_cleanup()
{
  // cleanup dead entities
  this->get_entities().cleanup();
}

//==============================================================================
void ThingSystem::on_event(ModuleEvent& event)
{
  switch (event.type)
  {
    case ModuleEventType::post_init:
    {
      this->post_init();
      break;
    }

    case ModuleEventType::pre_terminate:
    {
      this->pre_terminate();
      break;
    }

    case ModuleEventType::cleanup:
    {
      this->on_cleanup();
      break;
    }

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
      }
      break;
    }

    case ModuleEventType::game_update:
    {
      NC_SCOPE_PROFILER(ThingSystemUpdate)

#ifdef NC_DEBUG_DRAW
      this->do_raycast_debug();
#endif

      auto& entity_system = this->get_entities();

      //INPUT PHASE
      GameInputs curr_input = InputSystem::get().get_inputs();
      GameInputs prev_input = InputSystem::get().get_prev_inputs();

      // Handle the player first
      this->get_player()->update(curr_input, prev_input, event.update.dt);

      // Handle enemies
      entity_system.for_each<Enemy>([&](Enemy& enemy)
      {
        enemy.update(event.update.dt);
      });

      // Handle projectiles
      entity_system.for_each<Projectile>([&](Projectile& proj)
      {
        proj.update(event.update.dt);
      });

      break;
    }
  }
}

//==========================================================
Player* ThingSystem::get_player()
{
  return this->get_entities().get_entity<Player>(player_id);
}

//==============================================================================
EntityRegistry& ThingSystem::get_entities()
{
  nc_assert(entities);
  return *entities;
}

//==============================================================================
const EntityRegistry& ThingSystem::get_entities() const
{
  return const_cast<ThingSystem*>(this)->get_entities();
}

//==============================================================================
SaveGameData ThingSystem::save_game() const
{
  SaveGameData save;
  save.last_level = this->get_level_name();
  save.time       = SaveGameData::Clock::now();
  save.id         = ++last_save_id;
  return save;
}

//==============================================================================
void ThingSystem::load_game(const SaveGameData& save)
{
  this->request_level_change(save.last_level);
}

//==============================================================================
ThingSystem::SaveDatabase& ThingSystem::get_save_game_db()
{
  return save_db;
}

//==============================================================================
LevelName ThingSystem::get_level_name() const
{
  return level_name;
}

//==============================================================================
void ThingSystem::request_level_change(LevelName new_level)
{
  // Another level already scheduled.
  nc_assert(this->scheduled_level_id == INVALID_LEVEL_NAME);

  this->scheduled_level_id = new_level;
}

//==============================================================================
const MapSectors& ThingSystem::get_map() const
{
  nc_assert(map);
  return *map;
}

//==============================================================================
const SectorMapping& ThingSystem::get_sector_mapping() const
{
  nc_assert(mapping);
  return *mapping;
}

//==============================================================================
PhysLevel ThingSystem::get_level() const
{
  return PhysLevel
  {
    .entities = this->get_entities(),
    .map      = this->get_map(),
    .mapping  = this->get_sector_mapping(),
  };
}

//==============================================================================
EntityAttachment& ThingSystem::get_attachment_mgr()
{
  nc_assert(this->attachment);
  return *this->attachment;
}

//==============================================================================
const EntityAttachment& ThingSystem::get_attachment_mgr() const
{
  return const_cast<ThingSystem*>(this)->get_attachment_mgr();
}

//==============================================================================
void ThingSystem::cleanup_map()
{
  entities.reset();
  mapping.reset();
  map.reset();
  player_id = INVALID_ENTITY_ID;
}

//==============================================================================
void ThingSystem::build_map(LevelName level)
{
  nc_assert(!map && !mapping && !entities);

  map        = std::make_unique<MapSectors>();
  mapping    = std::make_unique<SectorMapping>(*map);
  entities   = std::make_unique<EntityRegistry>();
  attachment = std::make_unique<EntityAttachment>(*entities);

  entities->add_listener(mapping.get());
  entities->add_listener(attachment.get());

  map_helpers::load_json_map(level, *map, *mapping, *entities, player_id);
}

//==============================================================================
#ifdef NC_DEBUG_DRAW
void ThingSystem::do_raycast_debug()
{
  Player* player = this->get_player();
  if (player && player->get_camera() && CVars::debug_player_raycasts)
  {
    const Camera* camera = player->get_camera();
    const auto& lvl = ThingSystem::get().get_level();

    constexpr f32 RAY_LEN = 10.0f;
	
    const vec3 eye_pos  = camera->get_position();
    const vec3 look_dir = camera->get_forward();
	
    const vec3 ray_start = eye_pos;
    const vec3 ray_end   = eye_pos + look_dir * RAY_LEN;

    PhysLevel::Portals portals;
	
    CollisionHit hit = lvl.cylinder_cast_3d
    (
      ray_start, ray_end, 0.25f, 0.25f, 0, &portals
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
    }
  }
}
#endif

}
