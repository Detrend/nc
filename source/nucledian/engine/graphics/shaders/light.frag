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
    float constant;
    float linear;
    float quadratic;
};

struct TileData
{
    uint count;
    uint offset;
};

in vec2 uv;

out vec4 out_color;

layout(binding = 0) uniform sampler2D g_position;
layout(binding = 1) uniform sampler2D g_normal;
layout(binding = 2) uniform sampler2D g_stitched_normal;
layout(binding = 3) uniform sampler2D g_albedo;

layout(location = 0) uniform vec3 view_position;
layout(location = 1) uniform uint num_dir_lights;
layout(location = 2) uniform uint num_tiles_x;

layout(std430, binding = 0) readonly buffer dir_lights_buffer { DirLight dir_lights[]; };
layout(std430, binding = 1) readonly buffer point_light_buffer { PointLight point_lights[]; };
layout(std430, binding = 2) readonly buffer light_index_buffer { uint light_indices[]; };
layout(std430, binding = 3) readonly buffer tile_data_buffer { TileData tile_data[]; };

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

  float ambient_strength = 0.3f;
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

    vec3 light_direction = light.position - position;
    float distance = length(light_direction);
    light_direction = normalize(light_direction);

    float angle = dot(stitched_normal, light_direction) + float(billboard);

    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 diffuse = max(angle, 0.0f) * albedo;

    vec3 reflect_direction = reflect(-light_direction, stitched_normal);
    vec3 specular = specular_strength * pow(max(dot(view_direction, reflect_direction), 0.0f), shininess) * vec3(1.0f);

    final_color += (diffuse + specular) * light.color * light.intensity * attenuation;
  }
  
  out_color = vec4(final_color, 1.0f);
}

)";