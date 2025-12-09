// Project Nucledian Source File
#pragma once

#include <config.h> // NC_DEBUG_DRAW

#include <engine/core/engine_module_id.h>
#include <engine/core/engine_module.h>

#include <engine/entity/entity_types.h>
#include <engine/player/level_types.h>
#include <engine/player/save_types.h>

#include <game/game_types.h>
#include <math/vector.h>

#include <memory> // std::unique_ptr

namespace nc
{

struct ModuleEvent;
struct MapSectors;
struct MapDynamics;
struct GameInputs;
struct SectorMapping;
struct PhysLevel;
class  EntityRegistry;
class  Player;
class  EntityAttachment;
class  Projectile;

class GameSystem : public IEngineModule
{
public:
  struct SaveDbEntry
  {
    SaveGameData data;
    bool         dirty = false;
  };

  using MappingPtr    = std::unique_ptr<SectorMapping>;
  using MapPtr        = std::unique_ptr<MapSectors>;
  using DynamicsPtr   = std::unique_ptr<MapDynamics>;
  using RegistryPtr   = std::unique_ptr<EntityRegistry>;
  using AttachmentPtr = std::unique_ptr<EntityAttachment>;
  using SaveDatabase  = std::vector<SaveDbEntry>;

public:
  static EngineModuleId get_module_id();
  static GameSystem&   get();

  GameSystem();
  ~GameSystem();

  GameSystem(const GameSystem&)            = delete;
  GameSystem& operator=(const GameSystem&) = delete;

  bool init();
  void on_event(ModuleEvent& event) override;

  // Saving and loading the game. 
  SaveGameData save_game() const;
  void         load_game(const SaveGameData& save);

  SaveDatabase& get_save_game_db();

  // Level transition
  LevelName get_level_name() const;
  void      request_level_change(LevelName new_level);

  Player*                 get_player();
  EntityRegistry&         get_entities();
  MapDynamics&            get_map_dynamics();
  const EntityRegistry&   get_entities()       const;
  const MapSectors&       get_map()            const;
  const SectorMapping&    get_sector_mapping() const;
  PhysLevel               get_level()          const;
  EntityAttachment&       get_attachment_mgr();
  const EntityAttachment& get_attachment_mgr() const;

  // Helper for creating projectiles
  Projectile* spawn_projectile
  (
    ProjectileType type, vec3 point, vec3 dir, EntityID author
  );

private:
  void check_player_attack
  (
    const GameInputs&  curr_inputs,
    const GameInputs&  prev_inputs,
    const ModuleEvent& event
  );

  // Clean up the current map, entities, mapping etc..
  void cleanup_map();

  // Loads a new level with the given ID. Requires
  // "cleanup_map" to be called before. Do not call
  // this during the frame as it might cause bad stuff
  // to happen.
  void build_map(LevelName level);

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
  DynamicsPtr    dynamics;
  AttachmentPtr  attachment;
  RegistryPtr    entities;
  LevelName      level_name         = INVALID_LEVEL_NAME;
  LevelName      scheduled_level_id = INVALID_LEVEL_NAME;
  SaveDatabase   save_db;
  mutable SaveID last_save_id = 0;
};

}