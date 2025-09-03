constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core

struct TextureData {
  vec2 pos;
  vec2 size;
  float in_game_atlas;
};

in vec3 stitched_position;

in vec3 position;
in vec3 normal;
in float cumulative_wall_len;

flat in int texture_id;
flat in float texture_scale;
flat in float texture_rotation;
flat in float tile_rotations_count;
flat in float tile_rotation_increment;
flat in vec2 texture_offset;

layout(location = 0) out vec4 g_position;
layout(location = 1) out vec4 g_normal;
layout(location = 2) out vec4 g_albedo;

layout(location = 2) uniform vec2 game_atlas_size;
layout(location = 3) uniform vec2 level_atlas_size;
layout(location = 4) uniform sampler2D game_atlas_sampler;
layout(location = 5) uniform sampler2D level_atlas_sampler;

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
  vec2 uv = abs(normal.y) > 0.99f ? position.xz : vec2(cumulative_wall_len, position.y);
  // get indices of the tile we are currently draving
  vec2 rand_seed = vec2(floor(uv.x/texture_scale), floor(uv.y/texture_scale));
  // run those indices through a hash function
  float rand_value = floor(rand(rand_seed) * tile_rotations_count);
  // based on the random value we got, randomize the tile rotation
  float actual_rotation = texture_rotation + (rand_value * tile_rotation_increment);
  
  // floor uv is mirrored
  if (normal.y > 0.0f) uv.x *= -1.0f;
  // apply rotation, scale, and offset
  float c = cos(actual_rotation);
  float s = sin(actual_rotation);
  uv = (mat2(c, s, -s, c) * uv) / texture_scale + texture_offset;
  // tile texture
  uv = fract(uv);
  // flip y axis
  uv.y = 1.0f - uv.y;
  // compute atlas uv
  uv = (uv * texture_data.size + texture_data.pos) / atlas_size;

  vec4 color = texture(use_game_atlas ? game_atlas_sampler : level_atlas_sampler, uv);
  if (color.a == 0.0f)
    discard;

  // Position G-bugger works in camera local space in order to overcome portals space discontinuity.
  g_position.xyz = stitched_position;
  // 4-th component of position is used to determine if pixel should be lit
  g_position.w = 1.0f;
  g_normal.xyz = normalize(normal);
  // 4-th component of normal is used for specular strength
  g_normal.w = 0.0f;
  g_albedo = color;
}

)";