#include <engine/player/map_object.h>
#include <engine/core/engine.h>
#include <engine/map/map_system.h>
#include <engine/player/thing_system.h>
#include <engine/map/physics.h>

#include <math/lingebra.h>

namespace nc
{
  // ===============================================================================

  MapObject::MapObject(vec3 position, f32 width, f32 height, bool collision)
  : Entity(position, width, height)
  {
    this->collision = collision;
  }

  // ===============================================================================

  bool MapObject::did_collide(const MapObject& collider)
  {
    // EACH OBJECT HAS AN AABB COLLISION HULL

    if (!collision || !collider.collision)
    {
      return false;
    }

    vec3 position = Entity::get_position();
    f32  width    = Entity::get_radius();
    f32  height   = Entity::get_radius();

    // get distance from top down

    f32 difX = abs(position.x - collider.get_position().x);
    f32 difZ = abs(position.z - collider.get_position().z);

    //f32 topDownDistance = sqrtf(difX * difX + difY * difY);

    /*if (topDownDistance > width + collider.width) 
    {
      return false;
    }*/

    // AABB
    if (difX > width + collider.get_width() + 0.01f || difZ > width + collider.get_width() + 0.01f)
    {
      return false;
    }

    // check whether the bottom point of collider is in range of our position - height
    if (position.y <= collider.get_position().y && collider.get_position().y <= position.y + height) 
    {
      return true;
    }

    // check whether the top point of collider is in range of our position - height
    if (position.y <= collider.get_position().y + height && collider.get_position().y + height <= position.y + height)
    {
      return true;
    }

    return false;
  }

  // ===============================================================================

  f32 MapObject::get_width() const
  {
    return Entity::get_radius();
  }

  // ===============================================================================

  MapObject::~MapObject()
  {
  }
}