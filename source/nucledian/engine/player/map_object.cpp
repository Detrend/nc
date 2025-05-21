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

  void MapObject::check_collision(const MapObject& collider, vec3& velocity, f32 delta_seconds)
  {
    vec3 velocity_per_frame = velocity * delta_seconds;

    this->set_position(this->get_position() + velocity_per_frame);

    if (!did_collide(collider))
    {
      this->set_position(this->get_position() - velocity_per_frame);
      return;
    }

    f32 width = this->get_width();

    vec3 target_dist = collider.get_position() - this->get_position();

    vec3 intersect_union = vec3(get_width(), 0, get_width()) - (target_dist - vec3(collider.get_width(), 0, collider.get_width()));

    this->set_position(this->get_position() - velocity_per_frame);

    vec3 new_velocity = velocity_per_frame - intersect_union;
    vec3 mult = vec3(velocity_per_frame.x / (1 - new_velocity.x), 0, velocity_per_frame.z / (1 - new_velocity.z));

    if (mult.x < 0.05f) mult.x = 0;
    if (mult.z < 0.05f) mult.z = 0;

    target_dist = collider.get_position() - this->get_position();
    target_dist.x = abs(target_dist.x);
    target_dist.z = abs(target_dist.z);

    if (target_dist.x >= width + collider.get_width())
    {
      mult.z = 1;
    }

    if (target_dist.z >= width + collider.get_width())
    {
      mult.x = 1;
    }

    velocity.x = velocity_per_frame.x * mult.x / delta_seconds;
    velocity.z = velocity_per_frame.z * mult.z / delta_seconds;
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