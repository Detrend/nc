// Project Nucledian Source File
#pragma once

#include <types.h>
#include <math/vector.h>
#include <engine/entity/entity_types.h>

namespace nc
{

struct IEntityListener
{
  virtual ~IEntityListener() {};

  // Called when an entity moves or it's height/radius are changed.
  virtual void on_entity_move(EntityID id, vec3 pos, f32 r, f32 h) = 0;

  // Called just after the entity is marked as garbage. The entity is still
  // alive and can be accessed.
  // Called only the first time the entity is marked as garbage during the frame.
  virtual void on_entity_garbaged(EntityID id) = 0;

  // Called during cleanup on the end of the frame just before the entity
  // is destroyed.
  virtual void on_entity_destroy(EntityID id) = 0;

  // Called after creating and setting up an entity. At this point it can be
  // accessed.
  virtual void on_entity_create(EntityID id, vec3 pos, f32 r, f32 h) = 0;
};

}
