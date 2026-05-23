// Project Nuclidean Source File
#pragma once

#include <engine/entity/entity.h>

namespace nc
{

class Teleport : public Entity
{
public:
  static EntityType get_type_static();

  void init(vec3 position, EntityID to_teleport);

  void post_init();
  void update(f32 delta);

private:
  f32      time_remaining = 0.0f;
  EntityID entity;
};

}
