constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core

#define TILE_SIZE_X 16
#define TILE_SIZE_Y 16

#define LIGHT_BANDS 48
// #define DO_SHADOWS
// #define PIXEL_DEBUG

struct DirLight
{
  vec3  color;
  float intensity;
  vec3  direction;
};

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

struct TileData
{
  uint count;
  uint offset;
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

out vec4 out_color;

layout(binding = 0) uniform sampler2D  g_position;
layout(binding = 1) uniform sampler2D  g_stitched_position;
layout(binding = 2) uniform sampler2D  g_normal;
layout(binding = 3) uniform sampler2D  g_stitched_normal;
layout(binding = 4) uniform sampler2D  g_albedo;
layout(binding = 5) uniform usampler2D g_sector;

layout(location = 0) uniform vec3  view_position;
layout(location = 1) uniform uint  num_dir_lights;
layout(location = 2) uniform uint  num_tiles_x;
layout(location = 3) uniform float ambient_strength;
layout(location = 4) uniform uint  num_sectors;
layout(location = 5) uniform uint  num_walls;
layout(location = 6) uniform bool  do_shadows = true;

layout(std430, binding = 0) readonly buffer dir_lights_buffer      { DirLight   dir_lights[];       };
layout(std430, binding = 1) readonly buffer point_light_buffer     { PointLight point_lights[];     };
layout(std430, binding = 2) readonly buffer light_index_buffer     { uint       light_indices[];    };
layout(std430, binding = 3) readonly buffer tile_data_buffer       { TileData   tile_data[];        };
layout(std430, binding = 4) readonly buffer sector_data_buffer     { SectorData sectors[];          };
layout(std430, binding = 5) readonly buffer wall_data_buffer       { WallData   walls[];            };
layout(std430, binding = 6) readonly buffer portal_matrices_buffer { mat4       portal_matrices[]; };
layout(std430, binding = 7) readonly buffer sector_matrices_buffer { mat4       sector_matrices[]; };

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

bool is_in_shadow(vec3 position, vec3 stitched_position, uint start_sector_id, uint start_matrix_id, PointLight light)
{
  const uint INVALID_WALL_ID = 65535;
  const uint MAX_LIGHT_TRAVERSE_SECTORS = 32;

  vec3 ray_stitched_origin = stitched_position;
  vec3 ray_origin = position;
  vec3 ray_stitched_direction = normalize(light.stitched_position - stitched_position);
  mat4 stitched_to_local = inverse(sector_matrices[start_matrix_id]);
  vec3 ray_direction = mat3(stitched_to_local) * ray_stitched_direction;
  
  uint current_sector_id = start_sector_id;
  float ray_t = -0.001f;

  for (int hop = 0; hop < MAX_LIGHT_TRAVERSE_SECTORS; ++hop)
  {
    SectorData sector = sectors[current_sector_id];
    uint wall_index = INVALID_WALL_ID;

    if (current_sector_id == light.sector_id)
    {
      vec3 light_local = (stitched_to_local * vec4(light.stitched_position, 1.0)).xyz;
      vec3 diff = light_local - light.position;
      if (dot(diff, diff) > 0.01)
        return true;

      //float ray_hit_y = ray_origin.y + ray_t * ray_direction.y;
      //if (ray_hit_y < sector.floor_y - 0.001f || ray_hit_y > sector.ceil_y + 0.001f)
      //  return true;

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

    // check if ray can pass through the portal opening
    SectorData dest_sector = sectors[current_sector_id];
    if (ray_hit_y < dest_sector.floor_y - 0.001f || ray_hit_y > dest_sector.ceil_y + 0.001f)
      return true;

    mat4 portal_matrix = inverse(portal_matrices[wall.portal_matrix_index]);
    ray_origin = (portal_matrix * vec4(ray_origin, 1.0f)).xyz;
    ray_direction = normalize(mat3(portal_matrix) * ray_direction);
    stitched_to_local = portal_matrix * stitched_to_local;
  }

  max_hops_reached = true;
  return false;
}

void main()
{
  vec4 g_position_sample = texture(g_position, uv);
  vec3 position = g_position_sample.xyz;
  // 4-th component of position is used for specular strength
  float specular_strength = g_position_sample.w;

  vec4 g_stitched_position_sample = texture(g_stitched_position, uv);
  vec3 stitched_position = g_stitched_position_sample.xyz;
  uint matrix_id = floatBitsToUint(g_stitched_position_sample.w);

  vec4 g_normal_sample = texture(g_normal, uv);
  vec3 normal = g_normal_sample.xyz;
  // 4-th component of normal is used to determine if pixel should is a billboard
  bool billboard = g_normal_sample.w == 0.0f;

  vec4 g_stitched_normal_sample = texture(g_stitched_normal, uv);
  // zero for billboards
  vec3 stitched_normal = g_stitched_normal_sample.xyz;
  // 4-th component of stitched_normal is used to determine if shadows are enabled
  bool enable_shadows = g_stitched_normal_sample.w == 1.0f;

  vec3 albedo = texture(g_albedo, uv).rgb;
  uint sector_id = texture(g_sector, uv).x;

#define VOX_CNT 16
#ifdef DO_LIGHT_VOXELS
  position = round(position * VOX_CNT) / VOX_CNT;
  stitched_position = round(stitched_position * VOX_CNT) / VOX_CNT;
#endif

  const int shininess = 128;

  vec3 view_direction = normalize(view_position - stitched_position);
  vec3 final_color = ambient_strength * albedo;

  // directional lights
  for (int i = 0; i < num_dir_lights; i++)
  {
    vec3 reflect_direction = reflect(-dir_lights[i].direction, normal);

    // this is 0 for billboards
    float angle = dot(normal, dir_lights[i].direction);
    vec3 diffuse = max(angle, 0.0f) * albedo;
    vec3 specular = specular_strength * pow(max(dot(view_direction, reflect_direction), 0.0f), shininess) * vec3(1.0f);

    final_color += (diffuse + specular) * dir_lights[i].color * dir_lights[i].intensity;
  }

  // point lights
  uvec2 tile_coords = uvec2(gl_FragCoord.xy) / uvec2(TILE_SIZE_X, TILE_SIZE_Y);
  uint tile_index = tile_coords.y * num_tiles_x + tile_coords.x;

  TileData data = tile_data[tile_index];

  for (int i = 0; i < data.count; i++)
  {
    uint light_index = light_indices[data.offset + i];

    PointLight light = point_lights[light_index];

    vec3 light_direction = light.stitched_position - stitched_position;
    float distance_squared = dot(light_direction, light_direction);

    if (distance_squared >= light.radius * light.radius)
      continue;

    float distance = sqrt(distance_squared);
    light_direction /= distance;
    float angle = dot(stitched_normal, light_direction) + float(billboard);
    if (angle <= 0.0f)
      continue;

    if (do_shadows && enable_shadows && is_in_shadow(position, stitched_position, sector_id, matrix_id, light)) 
    continue;

    vec3 diffuse = max(angle, 0.0f) * albedo;

    vec3 half_direction = normalize(light_direction + view_direction);
    vec3 specular = specular_strength * pow(max(dot(stitched_normal, half_direction), 0.0f), shininess) * vec3(1.0f);

    float attenuation = pow(max(light.radius - distance, 0.0f) / light.radius, light.falloff);
    final_color += (diffuse + specular) * light.color * light.intensity * attenuation;
  }
  
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