#pragma once
#include <types.h>
#include <math/vector.h>

#include <engine/entity/entity.h>

namespace nc {

  // definition of a generic map object, mainly collisions

  class MapObject : public Entity
  {
  public:
    MapObject(vec3 position, f32 width, f32 height, bool collision);

    virtual bool did_collide(const MapObject& collider);
    virtual void check_collision(const MapObject& collider, vec3& velocity, f32 delta_seconds);

    f32 get_width() const;

    ~MapObject();

  protected:
    bool  collision;
  };
}
