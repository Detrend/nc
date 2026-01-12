constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core

#define TILE_SIZE_X 16
#define TILE_SIZE_Y 16

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
  vec3  color;
  float radius;
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
  // TODO: useless, remove
  uint packed_normal;
  uint destination_sector;
};

in vec2 uv;

out vec4 out_color;

layout(binding = 0) uniform sampler2D  g_position;
layout(binding = 1) uniform sampler2D  g_normal;
layout(binding = 2) uniform sampler2D  g_stitched_normal;
layout(binding = 3) uniform sampler2D  g_albedo;
layout(binding = 4) uniform usampler2D g_sector;

layout(location = 0) uniform vec3  view_position;
layout(location = 1) uniform uint  num_dir_lights;
layout(location = 2) uniform uint  num_tiles_x;
layout(location = 3) uniform float ambient_strength;
layout(location = 4) uniform uint  num_sectors;
layout(location = 5) uniform uint  num_lights;

layout(std430, binding = 0) readonly buffer dir_lights_buffer  { DirLight   dir_lights[];    };
layout(std430, binding = 1) readonly buffer point_light_buffer { PointLight point_lights[];  };
layout(std430, binding = 2) readonly buffer light_index_buffer { uint       light_indices[]; };
layout(std430, binding = 3) readonly buffer tile_data_buffer   { TileData   tile_data[];     };
layout(std430, binding = 4) readonly buffer sector_data_buffer { SectorData sectors[];       };
layout(std430, binding = 5) readonly buffer sector_map_buffer  { uint       sector_map[];    };
layout(std430, binding = 6) readonly buffer wall_data_buffer   { WallData   walls[];         };

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
  vec2 wall_direction = wall_p1 - wall_p0;

  float denominator = cross(ray_direction, wall_direction);

  // rays are parallel
  if (abs(denominator) < 0.0001f)
    return -1.0f;

  float ray_t  = cross(wall_origin - ray_origin, wall_direction) / denominator;
  float wall_t = cross(wall_origin - ray_origin, ray_direction ) / denominator;

  // intersection occurs outside of wall this wall segment
  if (wall_t < 0.0f || wall_t > 1.0f || ray_t <= 0.0001f)
    return -1.0f;

  return ray_t;
}

bool is_in_shadow(vec3 position, uint start_sector_id, PointLight light)
{
  const uint INVALID_WALL_ID = 65535;
  const uint MAX_LIGHT_TRAVERSE_SECTORS = 32;

  vec3 ray_origin = position;
  vec3 ray_direction = light.position - position;

  uint current_sector_id = start_sector_id;
  float ray_t = 0.0f;

  for (int i = 0; i < MAX_LIGHT_TRAVERSE_SECTORS; ++i)
  {
    if (current_sector_id == light.sector_id)
      return false;

    SectorData sector = sectors[current_sector_id];
    uint wall_index;

    for (uint i = 0; i < sector.walls_count; ++i)
    {
      WallData wall = walls[sector.walls_offset + i];
      float t = get_intersection_t(ray_origin.xz, ray_direction.xz, wall.start, wall.end);

      // hit point is further away on ray than previous hit points (prevents going backwards)
      if (t > ray_t + 0.01f)
      {
        wall_index = i;
        ray_t = t;
        break;
      }
    }

    WallData wall = walls[sector.walls_offset + wall_index];
    
    // check if ray collides with floor or ceiling
    float ray_hit_y = ray_origin.y + ray_t * ray_direction.y;
    if (ray_hit_y < sector.floor_y || ray_hit_y > sector.ceil_y)
      return true;

    // wall is solid wall (not a portal)
    if (wall.destination_sector == INVALID_WALL_ID)
      return true;

    current_sector_id = wall.destination_sector;
  }

  return true;
}

void main()
{
  vec4 g_position_sample = texture(g_position, uv);
  vec3 position = g_position_sample.xyz;
  // 4-th component of position is used for specular strength
  float specular_strength = g_position_sample.w;

  vec4 g_normal_sample = texture(g_normal, uv);
  vec3 normal = g_normal_sample.xyz;
  // 4-th component of normal is used to determine if pixel should be lit
  bool billboard = g_normal_sample.w == 0.0f;

  // zero for billboards
  vec3 stitched_normal = texture(g_stitched_normal, uv).xyz;
  vec3 albedo = texture(g_albedo, uv).rgb;

  uint sector_id = texture(g_sector, uv).x;
  for (uint remmaped_id = 0; remmaped_id < sector_map.length(); ++remmaped_id)
  {
    if (sector_id == sector_map[remmaped_id])
    {
      sector_id = remmaped_id;
      break;
    }
  }

  int shininess = 128;

  vec3 view_direction = normalize(view_position - position);
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

    if (is_in_shadow(position, sector_id, light))
        continue;

    vec3 light_direction = light.position - position;
    float distance = length(light_direction);
    light_direction = normalize(light_direction);

    float angle = dot(stitched_normal, light_direction) + float(billboard);

    float attenuation = pow(max(light.radius - distance, 0.0f) / light.radius, light.falloff);

    vec3 diffuse = max(angle, 0.0f) * albedo;

    vec3 reflect_direction = reflect(-light_direction, stitched_normal);
    vec3 specular = specular_strength * pow(max(dot(view_direction, reflect_direction), 0.0f), shininess) * vec3(1.0f);

    final_color += (diffuse + specular) * light.color * light.intensity * attenuation;
  }
  
  out_color = vec4(final_color, 1.0f);
}

)";