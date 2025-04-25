// Project Nucledian Source File
#pragma once

#include <engine/core/engine_module_id.h>
#include <engine/core/engine_module.h>

// MR says: remove this include once the entity system works properly. We
// do not want to include more than is necessary!!!
#include <engine/enemies/enemy.h>

#include <memory> // std::unique_ptr

namespace nc
{

struct ModuleEvent;
struct MapSectors;
struct GameInputs;

class Player;

class ThingSystem : public IEngineModule
{
public:
  using Enemies   = std::vector<Enemy>;
  using PlayerPtr = std::unique_ptr<Player>;
  using MapPtr    = std::unique_ptr<MapSectors>;

public:
  static EngineModuleId get_module_id();
  static ThingSystem&   get();

  bool init();
  void on_event(ModuleEvent& event) override;

  Player* get_player();

  const MapSectors& get_map()     const;
  const Enemies&    get_enemies() const;

  // TODO: remove later, only temporary
  void build_map();

private:
  void check_player_attack
  (
    const GameInputs&  curr_inputs,
    const GameInputs&  prev_inputs,
    const ModuleEvent& event
  );

  void check_enemy_attack(const ModuleEvent& event);

private:
  PlayerPtr player;
  MapPtr    map;
  Enemies   enemies;
};

}