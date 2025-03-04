#include <types.h>
#include <vec.h>
#include <vector_maths.h>

namespace nc {

  // definition of a generic map object, mainly collisions

  class mapObject
  {
  public:
    mapObject(vec3 position, f32 width, f32 height, bool collision);

    bool did_collide(mapObject collider);

    f32 get_widht();
    f32 get_height();
    vec3 get_position();

  protected:
    f32 width;
    f32 height;
    bool collision;

    vec3 position;
  };
}
