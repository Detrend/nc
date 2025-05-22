// Project Nucledian Source File
#pragma once

#include <engine/core/engine_module_id.h>
#include <engine/core/engine_module.h>

// MR says: remove this include once the entity system works properly. We
// do not want to include more than is necessary!!!
#include <engine/enemies/enemy.h>

#include <engine/entity/entity_types.h>

#include <memory> // std::unique_ptr
#include <engine/entity/item.h>

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
  using PickUps     = std::vector<PickUp>;
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

  const MapSectors&    get_map()            const;
  const SectorMapping& get_sector_mapping() const;

  // TODO: remove later, only temporary
  void build_map();

private:
  const Enemies&    get_enemies() const;
  void check_player_attack
  (
    const GameInputs&  curr_inputs,
    const GameInputs&  prev_inputs,
    const ModuleEvent& event
  );

  void check_enemy_attack(const ModuleEvent& event);

private:
  EntityID    player_id = INVALID_ENTITY_ID;
  MapPtr      map;
  MappingPtr  mapping;
  Enemies     enemies;
  PickUps     pickups;
  RegistryPtr entities;
};

}