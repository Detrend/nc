constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core

struct PointLight
{
  vec3  position;
  float intensity;
  vec3  stitched_position;
  float radius;
  vec3  color;
  float falloff;
  uint  sector_id;
};

struct SectorData
{
  float floor_y;
  float ceil_y;
  uint  walls_offset;
  uint  walls_count;
};

struct WallData
{
  vec2 start;
  vec2 end;
  uint packed_normal;
  uint destination;
  uint portal_matrix_index;
};

in vec2 uv;
in vec3 wp;
in vec3 normal;

layout(location = 3) uniform vec3 u_color;
layout(location = 8) uniform uint u_num_lights;
layout(location = 10) uniform uint num_sectors;
layout(location = 11) uniform uint num_walls;
layout(location = 12) uniform uint sector_id;

layout(std430, binding = 0) readonly buffer point_light_buffer      { PointLight point_lights[];     };
layout(std430, binding = 1) readonly buffer sector_data_buffer      { SectorData sectors[];          };
layout(std430, binding = 2) readonly buffer wall_data_buffer        { WallData   walls[];            };
layout(std430, binding = 3) readonly buffer portal_matricies_buffer { mat4       portal_matricies[]; };
layout(std430, binding = 4) readonly buffer sector_matricies_buffer { mat4       sector_matricies[]; };

bool out_of_range_sectors = false;
bool out_of_range_walls = false;
bool invalid_wall_id = false;
bool max_hops_reached = false;
bool debug_pixel = false;

float cross(vec2 a, vec2 b)
{
  return a.x * b.y - a.y * b.x;
}

// check intersection between top-down shadow ray and "wall ray"
// return t which tells at which point intersersection occurs
// if no intersection, returns -1.0f
float get_intersection_t(vec2 ray_origin, vec2 ray_direction, vec2 wall_p0, vec2 wall_p1)
{
  vec2 wall_origin = wall_p0;
  float wall_length = distance(wall_p1, wall_p0);
  vec2 wall_direction = normalize(wall_p1 - wall_p0);

  float denominator = cross(ray_direction, wall_direction);

  // rays are parallel
  if (abs(denominator) < 0.001f)
    return -1.0f;

  vec2 diff = wall_origin - ray_origin;
  float inv_denominator = 1.0f / denominator;

  float ray_t  = cross(diff, wall_direction) * inv_denominator;
  float wall_t = cross(diff, ray_direction ) * inv_denominator;

  // intersection occurs outside of this wall segment
  if (wall_t < -0.001f || wall_t > wall_length + 0.001f)
    return -1.0f;

  return ray_t;
}

bool is_in_shadow(vec3 position, vec3 stitched_position, uint start_sector_id, PointLight light)
{
  if (start_sector_id == light.sector_id)
    return false;

  const uint INVALID_WALL_ID = 65535;
  const uint MAX_LIGHT_TRAVERSE_SECTORS = 32;

  vec3 ray_stitched_origin = stitched_position;
  vec3 ray_origin = position;
  vec3 ray_stitched_direction = normalize(light.stitched_position - stitched_position);
  vec3 ray_direction = ray_stitched_direction;
  
  uint current_sector_id = start_sector_id;
  float ray_t = -0.001f;

  for (int hop = 0; hop < MAX_LIGHT_TRAVERSE_SECTORS; ++hop)
  {
    SectorData sector = sectors[current_sector_id];
    uint wall_index = INVALID_WALL_ID;

    if (current_sector_id == light.sector_id)
    {
      // check if ray collides with floor or ceiling
      float ray_hit_y = ray_origin.y + ray_t * ray_direction.y;
      if (ray_hit_y < sector.floor_y - 0.001f || ray_hit_y > sector.ceil_y + 0.001f)
        return true;

      return false;
    }

    if (current_sector_id >= num_sectors)
    {
      out_of_range_sectors = true;
      return false;
    }

    for (uint i = 0; i < sector.walls_count; ++i)
    {
      if (sector.walls_offset + i >= num_walls)
      {
        out_of_range_walls = true;
        return false;
      }
      WallData wall = walls[sector.walls_offset + i];

      vec2 wall_normal = unpackSnorm2x16(wall.packed_normal);
      // wall is facing opposite direction, we can skip it
      if (dot(wall_normal, ray_direction.xz) > 0.001f)
        continue;

      float t = get_intersection_t(ray_origin.xz, ray_direction.xz, wall.start, wall.end);
      if (t > ray_t)
      {
        wall_index = i;
        ray_t = t;
        break;
      }
    }

    if (wall_index == INVALID_WALL_ID)
    {
      invalid_wall_id = true;
      return false;
    }

    // check if ray collides with floor or ceiling
    float ray_hit_y = ray_origin.y + ray_t * ray_direction.y;
    if (ray_hit_y < sector.floor_y - 0.001f || ray_hit_y > sector.ceil_y + 0.001f)
      return true;
   
    WallData wall = walls[sector.walls_offset + wall_index];

    // wall is solid wall (not a portal)
    if (wall.destination == INVALID_WALL_ID)
      return true;

    current_sector_id = wall.destination;

    mat4 portal_matrix = inverse(portal_matricies[wall.portal_matrix_index]);
    ray_origin = (portal_matrix * vec4(ray_origin, 1.0f)).xyz;
    ray_direction = normalize(mat3(portal_matrix) * ray_direction);
  }

  max_hops_reached = true;
  return false;
}

out vec4 out_color;

void main()
{
  vec3 final_color = vec3(0.0);
  vec3 position = wp + normal * 0.1f;
  vec3 color = vec3(0.0);

  vec3 stitched_position = position;
  vec3 stitched_normal   = normal;

  for (int i = 0; i < u_num_lights; i++)
  {
    PointLight light = point_lights[i];

    vec3 light_direction = light.stitched_position - stitched_position;
    float distance_squared = dot(light_direction, light_direction);

    if (distance_squared >= light.radius * light.radius)
      continue;

    float distance = sqrt(distance_squared);
    light_direction /= distance;
    float angle = dot(stitched_normal, light_direction);
    if (angle <= 0.0f)
    continue;

#define ABC
#ifdef ABC
    if (is_in_shadow(position, stitched_position, sector_id, light))
      continue;
#endif

    float diffuse = max(angle, 0.0f);

    float attenuation = pow(max(light.radius - distance, 0.0f) / light.radius, light.falloff);
    final_color += diffuse * light.color * light.intensity * attenuation;
  }

//#define PIXEL_DEBUG
#ifdef PIXEL_DEBUG
if (debug_pixel)
  out_color = vec4(1.0, 1.0, 0.0, 1.0);
else if (out_of_range_sectors)
  out_color = vec4(1.0, 0.0, 0.0, 1.0);
else if (out_of_range_walls)
  out_color = vec4(0.0, 1.0, 0.0, 1.0);
else if (invalid_wall_id)
  out_color = vec4(0.0, 0.0, 1.0, 1.0);
else if (max_hops_reached)
  out_color = vec4(1.0, 0.0, 1.0, 1.0);
else
  out_color = vec4(final_color, 1.0f);
#else
  out_color = vec4(final_color, 1.0f);
#endif
}

)";
