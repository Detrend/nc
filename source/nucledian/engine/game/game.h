// Project Nucledian Source File
#pragma once

#include <types.h>
#include <engine/entity/entity_types.h>

#include <memory> // std::unique_ptr

namespace nc
{

struct PlayerSpecificInputs;
struct MapSectors;
struct MapDynamics;
struct SectorMapping;
class  EntityRegistry;
class  EntityAttachment;

struct Game
{
  // Simulates one frame of the game
  void update
  (
    f32 delta_time,
    PlayerSpecificInputs current_inputs,
    PlayerSpecificInputs previous_inputs
  );

  EntityID                          player_id = INVALID_ENTITY_ID;
  std::unique_ptr<MapSectors>       map;
  std::unique_ptr<SectorMapping>    mapping;
  std::unique_ptr<MapDynamics>      dynamics;
  std::unique_ptr<EntityRegistry>   entities;
  std::unique_ptr<EntityAttachment> attachment;
  u64                               frame_idx = 0;
  bool                              is_level_completed = false;
};

}
