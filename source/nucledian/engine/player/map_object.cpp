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
    // EACH OBJECT HAS A CYLINDER COLLISION HULL

    if (!collision)
    {
      return false;
    }

    // get distance from top down

    f32 difX = abs(position.x - collider.position.x);
    f32 difY = abs(position.y - collider.position.y);

    //f32 topDownDistance = sqrtf(difX * difX + difY * difY);

    /*if (topDownDistance > width + collider.width) 
    {
      return false;
    }*/

    // AABB
    if (difX > width || difY > width)
    {
      return false;
    }

    // check whether the bottom point of collider is in range of our position - height
    if (position.z <= collider.position.z && collider.position.z <= position.z + height) 
    {
      return true;
    }

    // check whether the top point of collider is in range of our position - height
    if (position.z <= collider.position.z + height && collider.position.z + height <= position.z + height)
    {
      return true;
    }

    return false;
  }

  // ===============================================================================

  f32 MapObject::get_widht()
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