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

#include <engine/graphics/graphics_system.h>

#include <game/projectile.h>

#include <common.h>

#include <intersect.h>

#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>

//==============================================================================
namespace nc::map_helpers
{ 

//==============================================================================
static void test_make_sector_height(
  f32                                         in_floor_y,
  f32                                         in_ceil_y,
  const std::vector<u16>&                     points,
  std::vector<map_building::SectorBuildData>& out,
  int                                         portal_wall_id    = -1,
  WallRelID                                   portal_wall_id_to = 0,
  SectorID                                    portal_sector     = INVALID_SECTOR_ID)
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
    });
  }

  out.push_back(map_building::SectorBuildData
  {
    .points  = std::move(walls),
    .floor_y = in_floor_y,
    .ceil_y  = in_ceil_y,
  });
}

//==============================================================================
static void test_make_sector
(
  const std::vector<u16>&                     points,
  std::vector<map_building::SectorBuildData>& out,
  int                                         portal_wall_id    = -1,
  WallRelID                                   portal_wall_id_to = 0,
  SectorID                                    portal_sector     = INVALID_SECTOR_ID)
{
  test_make_sector_height
  (
    MapSectors::SECTOR_FLOOR_Y,
    MapSectors::SECTOR_CEILING_Y,
    points, out, portal_wall_id,
    portal_wall_id_to, portal_sector
  );
}

//==============================================================================
[[maybe_unused]] static void make_cool_looking_map(MapSectors& map)
{
  // {22, 12, 13, 23}
  std::vector<vec2> points =
  {
    vec2{0	, 0},
    vec2{6	, 1},
    vec2{7	, 5},
    vec2{9	, 5},
    vec2{11	, 4},
    vec2{14	, 4},
    vec2{16	, 5},
    vec2{16	, 8},
    vec2{14	, 9},
    vec2{11	, 9},
    vec2{9	, 8},
    vec2{8	, 8},
    vec2{7	, 9},
    vec2{7	, 12},
    vec2{6	, 13},
    vec2{5	, 13},
    vec2{4	, 12},
    vec2{4	, 9},
    vec2{4	, 6},
    vec2{2	, 4},
    vec2{3	, 1},
    vec2{5	, 9},
    vec2{6	, 9},
    vec2{6	, 10},
    vec2{5	, 10},
    vec2{16	, 14},    // extra 4 pts
    vec2{18	, 14},
    vec2{18	, 18},
    vec2{16	, 18},
  };

  //constexpr vec2 max_range = {20.0f, 20.0f};
  // normalize the points
  //for (auto& pt : points)
  //{
  //  pt = ((pt / max_range) * 2.0f) - vec2{1.0f};
  //}

  // and then the sectors
  std::vector<map_building::SectorBuildData> sectors;

  test_make_sector({1, 2, 18, 19, 20}, sectors);
  test_make_sector({18, 2, 11, 12, 22, 21, 17}, sectors);
  test_make_sector({2, 3, 10, 11}, sectors);
  test_make_sector({3, 4, 5, 6, 7, 8, 9, 10}, sectors);
  test_make_sector({22, 12, 13, 23}, sectors, 1, 0, 8);    // this one
  test_make_sector({23, 13, 14, 15, 16, 24}, sectors);
  test_make_sector({17, 21, 24, 16}, sectors);
  test_make_sector({21, 22, 23, 24}, sectors);
  test_make_sector({25, 26, 27, 28}, sectors, 0, 1, 4);

  using namespace map_building::MapBuildFlag;

  map_building::build_map(
    points, sectors, map,
    //omit_convexity_clockwise_check |
    //omit_sector_overlap_check      |
    //omit_wall_overlap_check        |
    assert_on_fail);
}

//==============================================================================
[[maybe_unused]] static void make_random_square_maze_map(MapSectors& map, u32 size, u32 seed)
{
  constexpr f32 SCALING = 20.0f;

  std::srand(seed);

  std::vector<vec2> points;

  // first up the points
  for (u32 i = 0; i < size; ++i)
  {
    for (u32 j = 0; j < size; ++j)
    {
      const f32 x = ((j / static_cast<f32>(size-1)) * 2.0f - 1.0f) * SCALING;
      const f32 y = ((i / static_cast<f32>(size-1)) * 2.0f - 1.0f) * SCALING;
      points.push_back(vec2{x, y});
    }
  }

  // and then the sectors
  std::vector<map_building::SectorBuildData> sectors;

  for (u16 i = 0; i < size-1; ++i)
  {
    for (u16 j = 0; j < size-1; ++j)
    {
      auto rng = std::rand();
      if (rng % 4 != 0)
      {
        u16 i1 = static_cast<u16>(i * size + j);
        u16 i2 = i1+1;
        u16 i3 = static_cast<u16>((i+1) * size + j + 1);
        u16 i4 = i3-1;
        test_make_sector({i1, i2, i3, i4}, sectors);
      }
    }
  }

  using namespace map_building::MapBuildFlag;

  map_building::build_map(
    points, sectors, map,
    //omit_convexity_clockwise_check |
    //omit_sector_overlap_check      |
    //omit_wall_overlap_check        |
    assert_on_fail);
}

//==============================================================================
[[maybe_unused]] static void make_demo_map(MapSectors& map)
{
  std::vector<vec2> points =
  {
    vec2{0, 0},
    vec2{9, 0},
    vec2{9, 9},
    vec2{0, 9}, //
    vec2{2, 2},
    vec2{7, 2},
    vec2{7, 7},
    vec2{2, 7}, //
    vec2{3, 2},
    vec2{6, 2},
    vec2{7, 3},
    vec2{7, 6},
    vec2{6, 7},
    vec2{3, 7},
    vec2{2, 6},
    vec2{2, 3},  //
    vec2{13, 9}, // 16
    vec2{16, 9},
    vec2{16, 2},
    vec2{23, 2},
    vec2{23, 9},
    vec2{26, 9},
    vec2{26, 12},
    vec2{20, 12},
    vec2{20, 9},
    vec2{19, 9},
    vec2{19, 12},
    vec2{13, 12},
    vec2{19, 5},
    vec2{20, 5},
    vec2{20, 6},
    vec2{19, 6},
  };

  // and then the sectors
  std::vector<map_building::SectorBuildData> sectors;

  test_make_sector({0, 1, 5, 9,  8,  4}, sectors);
  test_make_sector({1, 2, 6, 11, 10, 5}, sectors, 3, 4, 9);
  test_make_sector({2, 3, 7, 13, 12, 6}, sectors);
  test_make_sector({3, 0, 4, 15, 14, 7}, sectors, 3, 4, 4);

  test_make_sector_height(0.2f, 5.0f, {16, 17, 25, 26, 27},     sectors, 4, 3, 3); // 4
  test_make_sector_height(0.3f, 2.0f, {25, 17, 31, 30, 20, 24}, sectors);
  test_make_sector_height(0.3f, 2.0f, {31, 17, 18, 28},         sectors);
  test_make_sector_height(0.3f, 2.0f, {18, 19, 29, 28},         sectors);
  test_make_sector_height(0.3f, 2.0f, {19, 20, 30, 29},         sectors);
  test_make_sector_height(0.2f, 5.0f, {22, 23, 24, 20, 21},     sectors, 4, 3, 1);

  using namespace map_building::MapBuildFlag;

  map_building::build_map(
    points, sectors, map,
    //omit_convexity_clockwise_check |
    //omit_sector_overlap_check      |
    //omit_wall_overlap_check        |
    assert_on_fail);
}

//==============================================================================
[[maybe_unused]] static void make_simple_portal_test_map(MapSectors& map)
{
  /*
  *     0-------1-------2
  *     |               |
  *     A               | <- sector 0
  *     |               |   
  *     3-------4   -   5------12
  *             |               |
  * sector 1 -> B       |       B <- sector 3
  *             |               |
  *     6-------7   -   8------13
  *     |               |
  *     A               | <- sector 2
  *     |               |
  *     9------10------11
  * 
  * note: its actually flip along horizontal axis
  */

  const vec2 offset = vec2(-1.0f, 0.5f);
  std::vector<vec2> points = {
    vec2(0, 0), //  0
    vec2(1, 0), //  1
    vec2(2, 0), //  2
    vec2(0, 1), //  3
    vec2(1, 1), //  4
    vec2(2, 1), //  5
    vec2(0, 2), //  6
    vec2(1, 2), //  7
    vec2(2, 2), //  8
    vec2(0, 3), //  9
    vec2(1, 3), // 10
    vec2(2, 3), // 11
    vec2(3, 1), // 12
    vec2(3, 2), // 13
  };

  for (auto& point : points)
  {
    point += offset;
  }

  std::vector<map_building::SectorBuildData> sectors;

  // sector 0
  test_make_sector({ 0, 1, 2, 5, 4, 3 }, sectors, 5, 5, 2);
  // sector 1
  test_make_sector({ 4, 5, 8, 7 }, sectors, 3, 1, 3);
  // sector 2
  test_make_sector({ 6, 7, 8, 11, 10, 9 }, sectors, 5, 5, 0);
  // sector 3
  test_make_sector({ 5, 12, 13, 8 }, sectors, 1, 3, 1);

  using namespace map_building::MapBuildFlag;

  map_building::build_map(
    points, sectors, map,
    //omit_convexity_clockwise_check |
    //omit_sector_overlap_check      |
    //omit_wall_overlap_check        |
    assert_on_fail);
}

}

namespace nc
{

constexpr cstr SAVE_DIR_RELATIVE = "save";
constexpr cstr SAVE_FILE_SUFFIX  = ".ncs";

//==========================================================
EngineModuleId ThingSystem::get_module_id()
{
  return EngineModule::entity_system;
}

//==========================================================
ThingSystem& ThingSystem::get()
{
  return get_engine().get_module<ThingSystem>();
}

//==========================================================
bool ThingSystem::init()
{
  // init level db
  for (LevelID id = 0; id < Levels::count; ++id)
  {
    LevelID next_lvl_id = (id+1 < Levels::count) ? id+1 : INVALID_LEVEL_ID;
    this->levels_db.push_back(LevelData
    {
      .next_level = next_lvl_id,
      .name       = LEVEL_NAMES[id],
    });
  }

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
  this->request_level_change(Levels::demo_map);
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
      if (this->scheduled_level_id != INVALID_LEVEL_ID)
      {
        get_engine().send_event
        (
          ModuleEvent{.type = ModuleEventType::before_map_rebuild}
        );

        // do level transition
        this->cleanup_map();
        this->build_map(this->scheduled_level_id);
        this->level_id = this->scheduled_level_id;
        this->scheduled_level_id = INVALID_LEVEL_ID;

        get_engine().send_event
        (
          ModuleEvent{.type = ModuleEventType::after_map_rebuild}
        );
      }
      break;
    }

    case ModuleEventType::game_update:
    {
      auto& entity_system = this->get_entities();

      //INPUT PHASE
      GameInputs curInputs = get_engine().get_module<InputSystem>().get_inputs();
      GameInputs prevInputs = get_engine().get_module<InputSystem>().get_prev_inputs();

      this->get_player()->get_wish_velocity(curInputs, event.update.dt);

      entity_system.for_each<Enemy>([&](Enemy& enemy)
      {
        enemy.get_wish_velocity(event.update.dt);
      });

      check_player_attack(curInputs, prevInputs, event);
      check_enemy_attack(event);

      //CHECK FOR COLLISIONS
      entity_system.for_each<Enemy>([&](Enemy& enemy)
      {
          this->get_player()->check_collision(enemy, this->get_player()->get_velocity(), event.update.dt);
      });

      entity_system.for_each<Enemy>([&](Enemy& enemy)
      {
        enemy.check_collision(*this->get_player(), enemy.get_velocity(), event.update.dt);

        entity_system.for_each<Enemy>([&](Enemy& enemy2)
        {
          if (enemy.get_id() != enemy2.get_id()) // a wrong way to check equality
          {
            enemy.check_collision(enemy2, enemy.get_velocity(), event.update.dt);
          }         
        });
      });

      //FINAL VELOCITY CHANGE
      this->get_player()->apply_velocity(event.update.dt);

      entity_system.for_each<Enemy>([&](Enemy& enemy)
      {
        enemy.apply_velocity();
      });

      // PROJECTILES
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
  save.last_level = this->get_level_id();
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
LevelID ThingSystem::get_level_id() const
{
  return level_id;
}

//==============================================================================
void ThingSystem::request_level_change(LevelID new_level)
{
  // Another level already scheduled.
  nc_assert(this->scheduled_level_id == INVALID_LEVEL_ID);

  this->scheduled_level_id = new_level;
}

//==============================================================================
void ThingSystem::request_next_level()
{
  nc_assert(this->level_id < this->levels_db.size());
  const auto next_id = this->levels_db[this->level_id].next_level;
  if (next_id == INVALID_LEVEL_ID)
  {
    return;
  }

  this->request_level_change(next_id);
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
const ThingSystem::LevelDatabase& ThingSystem::get_level_db() const
{
  return this->levels_db;
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
void ThingSystem::cleanup_map()
{
  enemies.clear();
  entities.reset();
  mapping.reset();
  map.reset();
  player_id = INVALID_ENTITY_ID;
}

//==========================================================
const ThingSystem::Enemies& ThingSystem::get_enemies() const
{
  return enemies;
}

//==========================================================
void ThingSystem::build_map(LevelID level)
{
  nc_assert(!map && !mapping && !entities);
  map      = std::make_unique<MapSectors>();
  mapping  = std::make_unique<SectorMapping>(*map);
  entities = std::make_unique<EntityRegistry>(*mapping);

  switch (level)
  {
    case Levels::demo_map:
      map_helpers::make_demo_map(*map);
      break;

    case Levels::cool_map:
      map_helpers::make_cool_looking_map(*map);
      break;

    case Levels::square_map:
      map_helpers::make_random_square_maze_map(*map, 32, 0);
      break;

    case Levels::portal_test:
      map_helpers::make_simple_portal_test_map(*map);
      break;

    default: nc_assert(false); break;
  }

  mapping->on_map_rebuild();

  auto* player = entities->create_entity<Player>(vec3{0.5, 0, 0.5});
  player_id = player->get_id();

  entities->create_entity<Enemy>(vec3{1, 0.0, 1}, FRONT_DIR);
  entities->create_entity<Enemy>(vec3{2, 0.0, 1}, FRONT_DIR);
  entities->create_entity<Enemy>(vec3{3, 0.0, 1}, FRONT_DIR);
}

//==========================================================
void ThingSystem::check_player_attack
(
  const GameInputs&  curr_inputs,
  const GameInputs&  prev_inputs,
  const ModuleEvent& event)
{
  auto& entity_system = this->get_entities();
  auto* player        = this->get_player();
  bool  didAttack     = player->get_attack_state(curr_inputs, prev_inputs, event.update.dt);

  if (didAttack)
  {
    [[maybe_unused]] vec3 rayStart = this->get_player()->get_position() + vec3(0, this->get_player()->get_view_height(), 0);
    [[maybe_unused]] vec3 rayEnd = rayStart + get_engine().get_module<GraphicsSystem>().get_camera()->get_forward() * 50.0f;

    [[maybe_unused]] f32 hitDistance = 999999;
    EntityID index = INVALID_ENTITY_ID;

    const auto wallHit = this->get_level().raycast3d(rayStart, rayEnd);
    f32 wallDist = wallHit ? wallHit.coeff : FLT_MAX;

    entity_system.for_each<Enemy>([&](Enemy& enemy)
    {
      const f32   width    = enemy.get_width();
      const f32   height   = enemy.get_height() * 2.0f;
      const vec3  position = enemy.get_position();

      const aabb3 bbox = aabb3
      {
        position - vec3{width, 0.0f,   width},
        position + vec3{width, height, width}
      };

      f32 out;
      vec3 normal;
      if (intersect::ray_aabb3(rayStart, rayEnd, bbox, out, normal))
      {
        if (hitDistance > out)
        {
          hitDistance = out;
          index = enemy.get_id();
        }
      }
    });

    if (index != INVALID_ENTITY_ID)
    {
      if (hitDistance < wallDist || !wallHit)
      {
        entity_system.get_entity<Enemy>(index)->damage(100);
      }
    }
  }

  if (didAttack)
  {
    auto dir = player->get_look_direction();

    // spawn projectile
    entity_system.create_entity<Projectile>
    (
      player->get_position() + UP_DIR * player->get_height() + dir * 0.3f,
      dir * 10.0f,
      0.15f,
      true
    );
  }
}

void ThingSystem::check_enemy_attack([[maybe_unused]] const ModuleEvent& event)
{
  auto& entity_system = this->get_entities();
  entity_system.for_each<Enemy>([&](Enemy& enemy)
  {
    if (enemy.can_attack())
    {
      [[maybe_unused]] vec3 rayStart = enemy.get_position() + vec3(0, 0.5f, 0);
      [[maybe_unused]] vec3 rayEnd = this->get_player()->get_position() + vec3(0, this->get_player()->get_view_height(), 0);

      const auto wallHit = this->get_level().raycast3d(rayStart, rayEnd);

      if (wallHit) 
      {
        return;
      }

      this->get_player()->Damage(10);
    }
  });
}

}

