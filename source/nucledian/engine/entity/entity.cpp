// Project Nucledian Source File
#include <common.h>

#include <engine/entity/entity.h>
#include <engine/entity/sector_mapping.h>
#include <engine/core/engine.h>
#include <engine/map/map_system.h>

#include <math/lingebra.h>

namespace nc
{

//==============================================================================
Entity::Entity(vec3 position, f32 radius, f32 height)
: m_Position(position)
, m_Radius2D(radius)
, m_Height(height)
{

}

//==============================================================================
Entity::~Entity()
{
  // TODO: unregister entity from the grid
}

//==============================================================================
EntityID Entity::get_id() const
{
  return m_IdAndType;
}

//==============================================================================
EntityType Entity::get_type() const
{
  return m_IdAndType.type;
}

//==============================================================================
vec3 Entity::get_position() const
{
  return m_Position;
}

//==============================================================================
void Entity::set_position(vec3 np)
{
  if (m_Position != np)
  {
    m_Position = np;
    m_Mapping->on_entity_move(m_IdAndType, m_Position, m_Radius2D, m_Height);
  }
}

//==============================================================================
f32 Entity::get_radius() const
{
  return m_Radius2D;
}

//==============================================================================
void Entity::set_radius(f32 r)
{
  nc_assert(r >= 0.0f);
  if (m_Radius2D != r)
  {
    m_Radius2D = r;
    m_Mapping->on_entity_move(m_IdAndType, m_Position, m_Radius2D, m_Height);
  }
}

//==============================================================================
f32 Entity::get_height() const
{
  return m_Height;
}

//==============================================================================
void Entity::set_height(f32 nh)
{
  nc_assert(nh >= 0.0f);
  if (m_Height != nh)
  {
    m_Height = nh;
    m_Mapping->on_entity_move(m_IdAndType, m_Position, m_Radius2D, m_Height);
  }
}

//==============================================================================
void Entity::set_pos_rad_height(vec3 p, f32 r, f32 h)
{
  if (p != m_Position || r != m_Radius2D || h != m_Height)
  {
    m_Position = p;
    m_Radius2D = r;
    m_Height   = h;
    m_Mapping->on_entity_move(m_IdAndType, m_Position, m_Radius2D, m_Height);
  }
}

Entity::Entity(vec3 position, f32 width, f32 height, bool collision)
  : Entity(position, width, height)
{
  this->collision = collision;
}

// ===============================================================================

bool Entity::did_collide(const Entity& collider)
{
  // EACH OBJECT HAS AN AABB COLLISION HULL

  if (!collision || !collider.collision)
  {
    return false;
  }

  vec3 position = Entity::get_position();
  f32  width = Entity::get_radius();
  f32  height = Entity::get_radius();

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

void Entity::check_collision(const Entity& collider, vec3& velocity, f32 delta_seconds)
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

f32 Entity::get_width() const
{
  return Entity::get_radius();
}

// ===============================================================================
/*static*/ void Entity::move(vec3& position, vec3& velocity, vec3& forward, f32 radius)
{
  const auto& map = get_engine().get_map();

  // MR says: For some reasong this solves some of our problems
  // with ending up stuck in a wall (caused by float inaccuracies?)
  constexpr f32 MAGIC = 1.01f;
  // MR says: The MAGIC constant actually causes some problems as well
  // and this solves them.
  u32 iterations_left = 12;

  while (iterations_left-- > 0)
  {
    vec2 out_n;
    f32  out_t;
    const auto from = position.xz();
    const auto dir = velocity.xz();

    const auto prev_velocity = velocity;

    if (map.raycast2d_expanded(from, from + dir, radius, out_n, out_t))
    {
      const auto remaining = dir * (1.0f - out_t);
      const auto projected = out_n * dot(remaining, out_n);
      const auto projected3 = vec3{ projected.x, 0.0f, projected.y };
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
  const auto ray_to = (position + velocity).xz();
  map.raycast2d_expanded(ray_from, ray_to, 0, __, _, &portals);

  const bool should_transform = portals.size();
  mat4 transformation = identity<mat4>();
  for (const auto& [wid, sid] : portals)
  {
    const auto trans = map.calculate_portal_to_portal_projection(sid, wid);
    transformation = trans * transformation;
  }

  if (should_transform)
  {
    position = (transformation * vec4{ position + velocity, 1.0f }).xyz();
    velocity = (transformation * vec4{ velocity, 0.0f }).xyz();
    forward = (transformation * vec4{ forward, 0.0f }).xyz();
  }
  else
  {
    position += velocity;
  }

  // change the height as well so we can move up the stairs
  auto sector_id = map.get_sector_from_point(position.xz());
  if (sector_id != INVALID_SECTOR_ID)
  {
    const f32 sector_floor_y = map.sectors[sector_id].floor_height;
    if (position.y < sector_floor_y)
    {
      position.y = sector_floor_y;
    }
  }
}

// ===============================================================================

}

