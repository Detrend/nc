// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <math/vector.h>
#include <engine/entity/entity_types.h>
#include <engine/input/game_input.h>
#include <engine/network/constants.h>
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

struct Game
{
  // Simulates one frame of the game
  void update
  (
    f32               delta_time,
    const InputArray& current_inputs,
    const InputArray& previous_inputs
  );

  void on_destroy();

  // Save/load data.
  // When loading, call this AFTER the map has been build.
  void serialize(Buffer& buffer);

  EntityID get_local_player_id() const;

  PlayerArray player_ids        = []() constexpr { PlayerArray array{}; array.fill(INVALID_ENTITY_ID); return array; }();
  vec3        spawn_point       = VEC3_ZERO;
  vec3        spawn_direction   = VEC3_X;

  std::unique_ptr<MapSectors>       map;
  std::unique_ptr<EntityRegistry>   entities;
  std::unique_ptr<SectorMapping>    mapping;
  std::unique_ptr<MapDynamics>      dynamics;
  std::unique_ptr<EntityAttachment> attachment;

  LevelTransitionData transition_data;
  u64                 frame_idx          = 0;
  f64                 time_since_start   = 0.0;
  bool                is_level_completed = false;
  LevelName           next_level_name    = INVALID_LEVEL_NAME;
};

}
