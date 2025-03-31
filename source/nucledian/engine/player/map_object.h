#pragma once
#include <types.h>
#include <math/vector.h>
#include <math/vector.h>

namespace nc {

  // definition of a generic map object, mainly collisions

  class MapObject
  {
  public:
    MapObject();
    MapObject(vec3 position, f32 width, f32 height, bool collision);

    virtual bool did_collide(MapObject collider);

    f32 get_width();
    f32 get_height();
    vec3 get_position();

    ~MapObject();

  protected:
    f32 width; //is actualy half a width
    f32 height;
    bool collision;

    vec3 position;
  };
}
