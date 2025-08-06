// Project Nucledian Source File
#pragma once

#include <config.h> // NC_DEBUG_DRAW

#include <engine/core/engine_module_id.h>
#include <engine/core/engine_module.h>

#include <engine/entity/entity_types.h>
#include <engine/player/level_types.h>
#include <engine/player/save_types.h>

#include <memory> // std::unique_ptr

namespace nc
{

struct ModuleEvent;
struct MapSectors;
struct GameInputs;
struct SectorMapping;
struct PhysLevel;
class  EntityRegistry;
class  Player;

class ThingSystem : public IEngineModule
{
public:
  struct SaveDbEntry
  {
    SaveGameData data;
    bool         dirty = false;
  };

  using MappingPtr    = std::unique_ptr<SectorMapping>;
  using MapPtr        = std::unique_ptr<MapSectors>;
  using RegistryPtr   = std::unique_ptr<EntityRegistry>;
  using LevelDatabase = std::vector<LevelData>;
  using SaveDatabase  = std::vector<SaveDbEntry>;

public:
  static EngineModuleId get_module_id();
  static ThingSystem&   get();

  bool init();
  void on_event(ModuleEvent& event) override;

  // Saving and loading the game. 
  SaveGameData save_game() const;
  void         load_game(const SaveGameData& save);

  SaveDatabase& get_save_game_db();

  // Level transition
  LevelID get_level_id() const;
  void    request_level_change(LevelID new_level);
  void    request_next_level();

  Player*               get_player();
  EntityRegistry&       get_entities();
  const EntityRegistry& get_entities()       const;
  const MapSectors&     get_map()            const;
  const SectorMapping&  get_sector_mapping() const;
  const LevelDatabase&  get_level_db()       const;
  PhysLevel             get_level()          const;

private:
  void check_player_attack
  (
    const GameInputs&  curr_inputs,
    const GameInputs&  prev_inputs,
    const ModuleEvent& event
  );

  void check_enemy_attack(const ModuleEvent& event);

  // Clean up the current map, entities, mapping etc..
  void cleanup_map();

  // Loads a new level with the given ID. Requires
  // "cleanup_map" to be called before. Do not call
  // this during the frame as it might cause bad stuff
  // to happen.
  void build_map(LevelID level);

  void post_init();

  void pre_terminate();

  void on_cleanup();

#ifdef NC_DEBUG_DRAW
  void do_raycast_debug();
#endif

private:
  EntityID       player_id = INVALID_ENTITY_ID;
  MapPtr         map;
  MappingPtr     mapping;
  RegistryPtr    entities;
  LevelID        level_id           = INVALID_LEVEL_ID;
  LevelID        scheduled_level_id = INVALID_LEVEL_ID;
  LevelDatabase  levels_db;
  SaveDatabase   save_db;
  mutable SaveID last_save_id = 0;
};

}