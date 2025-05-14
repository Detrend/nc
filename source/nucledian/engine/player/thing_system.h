// Project Nucledian Source File
#pragma once

#include <engine/core/engine_module_id.h>
#include <engine/core/engine_module.h>

// MR says: remove this include once the entity system works properly. We
// do not want to include more than is necessary!!!
#include <engine/enemies/enemy.h>

#include <engine/entity/entity_types.h>
#include <engine/player/level_types.h>

#include <memory> // std::unique_ptr

namespace nc
{

struct ModuleEvent;
struct MapSectors;
struct GameInputs;
struct SectorMapping;
class  EntityRegistry;
class  Player;

class ThingSystem : public IEngineModule
{
public:
  using Enemies     = std::vector<EntityID>;
  using MappingPtr  = std::unique_ptr<SectorMapping>;
  using MapPtr      = std::unique_ptr<MapSectors>;
  using RegistryPtr = std::unique_ptr<EntityRegistry>;

public:
  static EngineModuleId get_module_id();
  static ThingSystem&   get();

  bool init();
  void on_event(ModuleEvent& event) override;

  Player*         get_player();
  EntityRegistry& get_entities();

  LevelID get_level_id() const;
  void    request_level_change(LevelID new_level);

  const MapSectors&    get_map()            const;
  const SectorMapping& get_sector_mapping() const;

private:
  const Enemies&    get_enemies() const;
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

private:
  EntityID    player_id = INVALID_ENTITY_ID;
  MapPtr      map;
  MappingPtr  mapping;
  Enemies     enemies;
  RegistryPtr entities;
  LevelID     level_id           = INVALID_LEVEL_ID;
  LevelID     scheduled_level_id = INVALID_LEVEL_ID;
};

}