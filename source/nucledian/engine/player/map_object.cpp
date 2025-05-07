#include <engine/player/map_object.h>
#include <engine/core/engine.h>
#include <engine/map/map_system.h>

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
/*static*/ void MapObject::move(vec3& position, vec3& velocity, vec3& forward, f32 radius)
{
  const auto& map = get_engine().get_map();

  // MR says: For some reasong this solves some of our problems
  // with ending up stuck in a wall (caused by float inaccuracies?)
  constexpr f32 MAGIC = 1.01f; 
  // MR says: The MAGIC constant actually causes some problems as well
  // and this solves them.
  u32 iterations_left = 12; 

  while (iterations_left-->0)
  {
    vec2 out_n;
    f32  out_t;
    const auto from = position.xz();
    const auto dir  = velocity.xz();

    const auto prev_velocity = velocity;

    if (map.raycast2d_expanded(from, from + dir, radius, out_n, out_t))
    {
      const auto remaining  = dir * (1.0f - out_t);
      const auto projected  = out_n * dot(remaining, out_n);
      const auto projected3 = vec3{projected.x, 0.0f, projected.y};
      velocity -= MAGIC * projected3;
    }

    if (prev_velocity == velocity)
    {
      break;
    }
  }

  f32                 _;
  vec2                __;
  MapSectors::Portals portals;
  const auto ray_from = position.xz();
  const auto ray_to   = (position + velocity).xz();
  map.raycast2d_expanded(ray_from, ray_to, 0, __, _, &portals);

  const bool should_transform = portals.size();
  mat4 transformation = identity<mat4>();
  for (const auto&[wid, sid] : portals)
  {
    const auto trans = map.calculate_portal_to_portal_projection(sid, wid);
    transformation = trans * transformation;
  }

  if (should_transform)
  {
    position = (transformation * vec4{position + velocity, 1.0f}).xyz();
    velocity = (transformation * vec4{velocity, 0.0f}).xyz();
    forward  = (transformation * vec4{forward, 0.0f}).xyz();
  }
  else
  {
    position += velocity;
  }
}

  // ===============================================================================

  MapObject::~MapObject()
  {
  }
}