constexpr const char* FRAGMENT_SOURCE = R"(

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

void main()
{
  TextureData texture_data = textures[texture_id];
  bool use_game_atlas = texture_data.in_game_atlas == 1.0f;
  vec2 atlas_size = (use_game_atlas ? game_atlas_size : level_atlas_size);

  // uv based on world position
  vec2 uv_meters = abs(normal.y) > 0.99f ? position.xz : vec2(cumulative_wall_len, position.y);

  vec2 uv_pixels = vec2(ivec2((uv_meters + texture_offset) * PIXELS_PER_M));
  uv_pixels   = mod(uv_pixels, texture_data.size);
  vec2 uv_0_1 = clamp(uv_pixels / texture_data.size, vec2(0.001f), vec2(0.999f));
  if (normal.y > 0.0f) uv_0_1.x = 1.0f - uv_0_1.x; // flip floor x
  uv_0_1.y = 1.0f - uv_0_1.y; // flip, not sure why, but it was in the original code
  vec2 uv = (texture_data.pos + uv_0_1 * texture_data.size) / atlas_size;

  vec4 color;
  if (use_game_atlas) {
    color = texture(game_atlas_sampler, uv);
  } else{
    color = texture(level_atlas_sampler, uv);
  }
  if (color.a < 0.95f)
    discard;

  // Position G-bugger works in camera local space in order to overcome portals space discontinuity.
  g_position.xyz = position;
  // 4-th component of position is used for specular strength
  g_position.w = 0.0f;
  g_stitched_position = vec4(stitched_position, uintBitsToFloat(matrix_id));
  g_normal.xyz = normalize(normal);
  // 4-th component of normal is used to determine if pixel should be lit
  g_normal.w = 1.0f;
  g_stitched_normal.xyz = normalize(stitched_normal);
  // 4-th component of stitched_normal is used to determine if shadows are enabled
  g_stitched_normal.w = 1.0f;
  g_albedo = color;
  g_sector = sector_id;
}

)";