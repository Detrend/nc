// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <engine/entity/entity_types.h>
#include <engine/player/level_types.h>
#include <engine/player/save_types.h>

#include <memory> // std::unique_ptr

namespace nc
{

struct PlayerSpecificInputs;
struct MapSectors;
struct MapDynamics;
struct SectorMapping;
class  EntityRegistry;
class  EntityAttachment;
class  Buffer;

enum class Difficulty : u8
{
  IGN            = 0,
  medium         = 1,
  hard           = 2,
  ultra_violence = 3,
};

struct DifficultySettings
{
  f32 pickup_ammo;
  f32 pickup_health;
  f32 enemy_health;
  f32 enemy_damage;
  f32 enemy_reload_speed;
};

struct Game
{
  // Simulates one frame of the game
  void update
  (
    f32 delta_time,
    PlayerSpecificInputs current_inputs,
    PlayerSpecificInputs previous_inputs
  );

  void on_destroy();

  // Save/load data.
  // When loading, call this AFTER the map has been build.
  void serialize(Buffer& buffer);

  EntityID                          player_id = INVALID_ENTITY_ID;
  std::unique_ptr<MapSectors>       map;
  std::unique_ptr<EntityRegistry>   entities;
  std::unique_ptr<SectorMapping>    mapping;
  std::unique_ptr<MapDynamics>      dynamics;
  std::unique_ptr<EntityAttachment> attachment;
  LevelTransitionData               transition_data;
  Difficulty                        difficulty = Difficulty::medium;
  u64                               frame_idx = 0;
  f64                               time_since_start = 0.0;
  bool                              is_level_completed = false;
  LevelName                         next_level_name    = INVALID_LEVEL_NAME;
};

}
