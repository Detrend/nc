#include <engine/player/map_object.h>

namespace nc
{
  MapObject::MapObject()
  {
  }

  // ===============================================================================

  MapObject::MapObject(vec3 position, f32 width, f32 height, bool collision)
  {
    this->position = position;
    this->width = width;
    this->height = height;
    this->collision = collision;
  }

  // ===============================================================================

  bool MapObject::did_collide(MapObject collider)
  {
    // EACH OBJECT HAS AN AABB COLLISION HULL

    if (!collision && !collider.collision)
    {
      return false;
    }

    // get distance from top down

    f32 difX = abs(position.x - collider.position.x);
    f32 difZ = abs(position.z - collider.position.z);

    //f32 topDownDistance = sqrtf(difX * difX + difY * difY);

    /*if (topDownDistance > width + collider.width) 
    {
      return false;
    }*/

    // AABB
    if (difX > width + collider.width + 0.01f || difZ > width + collider.width + 0.01f)
    {
      return false;
    }

    // check whether the bottom point of collider is in range of our position - height
    if (position.y <= collider.position.y && collider.position.y <= position.y + height) 
    {
      return true;
    }

    // check whether the top point of collider is in range of our position - height
    if (position.y <= collider.position.y + height && collider.position.y + height <= position.y + height)
    {
      return true;
    }

    return false;
  }

  // ===============================================================================

  f32 MapObject::get_width()
  {
    return width;
  }

  // ===============================================================================

  f32 MapObject::get_height()
  {
    return height;
  }

  // ===============================================================================

  vec3 MapObject::get_position()
  {
    return position;
  }

  // ===============================================================================

  MapObject::~MapObject()
  {
  }
}