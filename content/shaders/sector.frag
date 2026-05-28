#version 430 core
#extension GL_NV_gpu_shader5 : enable

struct TextureData {
  vec2 pos;
  vec2 size;
  float in_game_atlas;
};

in vec3 position;
in vec3 stitched_position;
in vec3 normal;
in vec3 stitched_normal;
in float cumulative_wall_len;

flat in int texture_id;
flat in float texture_scale;
flat in float texture_rotation;
flat in float tile_rotations_count;
flat in float tile_rotation_increment;
flat in vec2 texture_offset;

layout(location = 0) out vec4 g_position;
layout(location = 1) out vec4 g_stitched_position;
layout(location = 2) out vec4 g_normal;
layout(location = 3) out vec4 g_stitched_normal;
layout(location = 4) out vec4 g_albedo;
layout(location = 5) out uint g_sector;

layout(binding = 0) uniform sampler2D game_atlas_sampler;
layout(binding = 1) uniform sampler2D level_atlas_sampler;
layout(binding = 2) uniform sampler2D game_normal_sampler;
layout(binding = 3) uniform sampler2D level_normal_sampler;
layout(binding = 4) uniform sampler2D game_specular_sampler;
layout(binding = 5) uniform sampler2D level_specular_sampler;

layout(location = 2) uniform vec2 game_atlas_size;
layout(location = 3) uniform vec2 level_atlas_size;
layout(location = 5) uniform uint sector_id;
layout(location = 6) uniform uint matrix_id;

#define PIXELS_PER_M 48

layout(std430, binding = 0) buffer texture_buffer {
    TextureData textures[];
};

// copypasted from: https://stackoverflow.com/questions/12964279/whats-the-origin-of-this-glsl-rand-one-liner
float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec3 perturb_normal(vec3 face_in, vec3 normal_ts)
{
  vec3 face = normalize(face_in);
  vec3 tangent;
  vec3 bitangent;
  if (abs(face.y) > 0.99f) {
    // floor or ceiling: uv_meters = position.xz, floor also flips x
    tangent   = vec3(face.y > 0.0f ? -1.0f : 1.0f, 0.0f, 0.0f);
    bitangent = vec3(0.0f, 0.0f, -1.0f);
  } else {
    // wall: uv_meters = (cumulative_wall_len, position.y)
    tangent   = normalize(cross(vec3(0.0f, 1.0f, 0.0f), face));
    bitangent = vec3(0.0f, -1.0f, 0.0f);
  }
  return normalize(tangent * normal_ts.x + bitangent * normal_ts.y + face * normal_ts.z);
}

void main()
{
  TextureData texture_data = textures[texture_id];
  bool use_game_atlas = texture_data.in_game_atlas == 1.0f;
  vec2 atlas_size = (use_game_atlas ? game_atlas_size : level_atlas_size);

  // uv based on world position
  vec2 uv_meters = abs(normal.y) > 0.99f ? position.xz : vec2(cumulative_wall_len, position.y);

  vec2 uv_pixels = vec2(ivec2((uv_meters + texture_offset) * PIXELS_PER_M));
  uv_pixels   = mod(uv_pixels, texture_data.size);
  vec2 uv_0_1 = clamp(uv_pixels / texture_data.size, vec2(0.01f), vec2(0.99f));
  if (normal.y > 0.0f) uv_0_1.x = 1.0f - uv_0_1.x; // flip floor x
  uv_0_1.y = 1.0f - uv_0_1.y; // flip, not sure why, but it was in the original code
  vec2 uv = (texture_data.pos + uv_0_1 * texture_data.size) / atlas_size;

  vec4 color;
  vec3 normal_rgb;
  float specular_strength;
  if (use_game_atlas) {
    color = texture(game_atlas_sampler, uv);
    normal_rgb = texture(game_normal_sampler, uv).xyz;
    specular_strength = texture(game_specular_sampler, uv).r;
  } else{
    color = texture(level_atlas_sampler, uv);
    normal_rgb = texture(level_normal_sampler, uv).xyz;
    specular_strength = texture(level_specular_sampler, uv).r;
  }
  //if (color.a < 0.95f)
  //  discard;

  vec3 normal_ts = normal_rgb * 2.0f - 1.0f;
  vec3 world_normal          = perturb_normal(normal, normal_ts);
  vec3 stitched_world_normal = perturb_normal(stitched_normal, normal_ts);

  // Position G-bugger works in camera local space in order to overcome portals space discontinuity.
  g_position.xyz = position;
  // 4-th component of position is used for specular strength
  g_position.w = specular_strength;
  g_stitched_position = vec4(stitched_position, uintBitsToFloat(matrix_id));
  g_normal.xyz = world_normal;
  // 4-th component of normal is used to determine if pixel should be lit
  g_normal.w = 1.0f;
  g_stitched_normal.xyz = stitched_world_normal;
  // 4-th component of stitched_normal is used to determine if shadows are enabled
  g_stitched_normal.w = 1.0f;
  g_albedo = color;
  g_sector = sector_id;
}
