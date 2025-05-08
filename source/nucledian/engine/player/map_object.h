#pragma once
#include <types.h>
#include <math/vector.h>
#include <math/vector.h>
#include <aabb.h>

#include <engine/entity/entity.h>

namespace nc {

  // definition of a generic map object, mainly collisions

  class MapObject : public Entity
  {
  public:
    MapObject(vec3 position, f32 width, f32 height, bool collision);

    virtual bool did_collide(const MapObject& collider);

    f32 get_width() const;

    static void move(vec3& position, vec3 velocity, vec3& forward, f32 radius);

    ~MapObject();

  protected:
    bool  collision;
    aabb3 bounds;
  };
}
