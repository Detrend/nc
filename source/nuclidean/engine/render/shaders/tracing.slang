#include "tracing.shared.h"

BUFFER(StructuredBuffer<Wall>,   walls);
BUFFER(StructuredBuffer<Sector>, sectors);
BUFFER(StructuredBuffer<Portal>, portals);

UNIFORM(ConstantBuffer<Context>, context);

IMAGE(RWTexture2D<float3>,       out_color);

[shader("compute")]
[numthreads(TRACING_GROUP_SIZE, TRACING_GROUP_SIZE, 1)]
void main(uvec2 pixel: SV_DispatchThreadID)
{
  uvec2 resolution;
  out_color.GetDimensions(resolution.x, resolution.y);
  if (any(pixel >= resolution)) return;

  // camera is outside of the map
  if (context.camera_sector_id == INVALID)
  {
    out_color[pixel] = float3(0.0f);
    return;
  }

  const vec3 forward = context.camera_direction;
  const vec3 right = normalize(cross(forward, vec3(0.0f, 1.0f, 0.0f)));
  const vec3 up = cross(right, forward);

  // dimensions of the virtual screen plane
  const f32 half_height = tan(FOV * 0.5f);
  const f32 half_width = half_height * (f32)resolution.x / (f32)resolution.y;

  // coordinate of pixel center within the virtual screen plane measured from it's center
  const f32 x = (((f32)pixel.x + 0.5f) / (f32)resolution.x * 2.0f - 1.0f) * half_width;
  const f32 y = (((f32)pixel.y + 0.5f) / (f32)resolution.y * 2.0f - 1.0f) * half_height;

  const vec3 ray_direction = forward + right * x + up * y;

  const Hit hit = trace(context.camera_position, ray_direction, context.camera_sector_id);
  out_color[pixel] = shade_hit(hit);
}

enum HitKind
{
  nothing,
  floor,
  ceiling,
  wall,
};

struct Hit
{
  HitKind kind;
  // Sector the ray was in when it hit.
  u32 sector_id;
  // Wall the ray hit, INVALID unless kind is wall.
  u32 wall_id;
  // Where the ray hit, after the portal transformations.
  vec3 position;
};

// Follows the ray through sectors and portals until it hits a floor, ceiling or wall.
Hit trace(vec3 ray_origin, vec3 ray_direction, u32 start_sector_id)
{
  const u32 MAX_RAY_HOPS = 256;

  u32 sector_id = start_sector_id;

  for (u32 hop = 0; hop < MAX_RAY_HOPS; ++hop)
  {
    const Sector sector = sectors[sector_id];
    const WallHit hit = intersect_sector(ray_origin.xz, ray_direction.xz, sector_id);

    // the ray reaches the floor or ceiling before the wall
    const f32 hit_y = ray_origin.y + hit.t * ray_direction.y;
    if (hit_y < sector.floor_y)
      return Hit(HitKind::floor, sector_id, INVALID, intersect_y(ray_origin, ray_direction, sector.floor_y));
    if (hit_y > sector.ceil_y)
      return Hit(HitKind::ceiling, sector_id, INVALID, intersect_y(ray_origin, ray_direction, sector.ceil_y));

    const vec3 hit_position = ray_origin + ray_direction * hit.t;

    const Wall wall = walls[hit.wall_id];
    if (wall.portal_id == INVALID)
      return Hit(HitKind::wall, sector_id, hit.wall_id, hit_position);

    const Portal portal = portals[wall.portal_id];
    const Sector destination = sectors[portal.destination_id];

    // the ray hits the part of the wall above or below the portal opening
    const f32 destination_y = hit_y + portal.translation.y;
    if (destination_y < destination.floor_y || destination_y > destination.ceil_y)
      return Hit(HitKind::wall, sector_id, hit.wall_id, hit_position);

    ray_origin = rotate_y(ray_origin, portal.rotation) + portal.translation;
    ray_direction = rotate_y(ray_direction, portal.rotation);
    sector_id = portal.destination_id;
  }

  return Hit(HitKind::nothing, sector_id, INVALID, ray_origin);
}

struct WallHit
{
  f32 t;
  u32 wall_id;
};

// Computes where the ray leaves the sector. Walls go counter-clockwise, so the exit wall
// starts on the right side of the ray and ends on its left side.
WallHit intersect_sector(vec2 ray_origin, vec2 ray_direction, u32 sector_id)
{
  const u32 walls_offset = sectors[sector_id].walls_offset;
  const u32 walls_count = sectors[sector_id].walls_count;

  for (u32 i = 0; i < walls_count; ++i)
  {
    const u32 wall_id = walls_offset + i;
    const vec2 start = walls[wall_id].position;
    const vec2 end = walls[walls_offset + (i + 1) % walls_count].position;

    const f32 start_side = side_of_ray(ray_origin, ray_direction, start);
    const f32 end_side = side_of_ray(ray_origin, ray_direction, end);

    if (start_side <= 0.0f && end_side > 0.0f)
    {
      const f32 fraction = start_side / (start_side - end_side);
      const vec2 exit_point = lerp(start, end, fraction);

      const f32 t = dot(exit_point - ray_origin, ray_direction) / dot(ray_direction, ray_direction);

      return WallHit(t, wall_id);
    }
  }

  return WallHit(0.0f, INVALID);
}

// Determine which side of the ray's line is the point on.
//   > 0 : left
//   < 0 : right
//   = 0 : on line
f32 side_of_ray(vec2 ray_origin, vec2 ray_direction, vec2 point)
{
  return cross2(ray_direction, point - ray_origin);
}

// 2D cross product (positive if b points to the left of a).
f32 cross2(vec2 a, vec2 b)
{
  return a.x * b.y - a.y * b.x;
}

// Point where the ray crosses the horizontal plane at height y.
vec3 intersect_y(vec3 ray_origin, vec3 ray_direction, f32 y)
{
  return ray_origin + ray_direction * ((y - ray_origin.y) / ray_direction.y);
}

// Rotates the vector around the Y axis.
vec3 rotate_y(vec3 v, f32 angle)
{
  const f32 c = cos(angle);
  const f32 s = sin(angle);
  return vec3(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
}

// Color of the surface the ray hit.
float3 shade_hit(Hit hit)
{
  if (hit.kind == HitKind::floor)
    return random_color(hit.sector_id);

  if (hit.kind == HitKind::ceiling)
    return random_color(hit.sector_id) * 0.5f;

  if (hit.kind == HitKind::wall)
    return random_color(hit.wall_id);

  return float3(1.0f, 0.0f, 1.0f);
}

// Color that differs for each id, so neighboring surfaces are easy to tell apart.
float3 random_color(u32 id)
{
  // lowbias32 integer hash by Chris Wellons
  u32 hash = id;
  hash ^= hash >> 16;
  hash *= 0x7feb352du;
  hash ^= hash >> 15;
  hash *= 0x846ca68bu;
  hash ^= hash >> 16;
  return float3(uint3(hash, hash >> 8, hash >> 16) & 0xFFu) / 255.0f;
}
