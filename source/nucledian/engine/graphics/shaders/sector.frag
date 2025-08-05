constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core

struct TextureData {
  vec2 pos;
  vec2 size;
  float in_game_atlas;
};

in vec3 position;
in vec3 normal;
in float cumulative_wall_len;
flat in int texture_id;
flat in float texture_scale;

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

void main()
{
  TextureData texture_data = textures[texture_id];
  bool use_game_atlas = texture_data.in_game_atlas == 1.0f;
  vec2 atlas_size = (use_game_atlas ? game_atlas_size : level_atlas_size);

  // TODO: floor have flipped x
  vec2 uv = abs(normal.y) > 0.99f ? position.xz : vec2(cumulative_wall_len, position.y);
  uv /= texture_scale;
  uv = fract(uv);
  uv.y = 1.0f - uv.y;
  uv = (uv * texture_data.size + texture_data.pos) / atlas_size;

  vec4 color = texture(use_game_atlas ? game_atlas_sampler : level_atlas_sampler, uv);
  if (color.a == 0.0f)
    discard;

  g_position.xyz = position;
  // 4-th component of position is used to determine if pixel should be lit
  g_position.w = 1.0f;
  g_normal.xyz = normalize(normal);
  // 4-th component of normal is used for specular strength
  g_normal.w = 0.0f;
  g_albedo = color;
}

)";