
#version 430 core
#extension GL_NV_gpu_shader5 : enable

layout(early_fragment_tests) in;

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
in      vec2 megatex_uv;

layout(location = 0) out vec4 g_position;
layout(location = 1) out vec4 g_stitched_position;
layout(location = 2) out vec4 g_normal;
layout(location = 3) out vec4 g_stitched_normal;
layout(location = 4) out vec4 g_albedo;
layout(location = 5) out uint g_sector;

layout(binding = 0) uniform sampler2D game_atlas_sampler;
layout(binding = 1) uniform sampler2D megatex_debug;
layout(binding = 2) uniform sampler2D megatex_input;
layout(binding = 3) uniform sampler2D megatex_shadows;

layout(location = 2) uniform vec2  game_atlas_size;
layout(location = 3) uniform vec2  level_atlas_size;
layout(location = 5) uniform uint  sector_id;
layout(location = 6) uniform uint  matrix_id;
layout(location = 7) uniform ivec4 u_tonemap;

layout(std430, binding = 0) buffer texture_buffer {
    TextureData textures[];
};

layout(binding = 7, r8) uniform image2D megatex_mask;

vec3 no_tonemap(vec3 x)
{
  return x;
}

vec3 aces(vec3 x)
{
  return clamp((x * (2.51*x + 0.03)) / (x * (2.43*x + 0.59) + 0.14), 0.0, 1.0);
}

vec3 reinhard(vec3 x)
{
  return x / (x + vec3(1.0f));
}

vec3 agx(vec3 x)
{
  // Prevent negative values
  x = max(x, 0.0);

  // --- Input transform ---
  // Approximate log2 encoding
  x = log2(1.0 + x);

  // Normalize expected HDR range
  // (roughly maps 0..16 stops into 0..1)
  x /= 10.0;

  // --- Sigmoid contrast curve ---
  vec3 y = x * x * (3.0 - 2.0 * x);

  // --- Highlight rolloff ---
  y = y / (1.0 + y);

  // --- Highlight desaturation ---
  float luma = dot(y, vec3(0.2126, 0.7152, 0.0722));

  // Amount of desaturation increases with brightness
  float desat = smoothstep(0.4, 1.0, luma);

  y = mix(y, vec3(luma), desat * 0.35);

  // --- Output gamma ---
  y = pow(y, vec3(1.0 / 2.2));

  return clamp(y, 0.0, 1.0);
}

vec3 tonemap(vec3 x)
{
  switch (u_tonemap.x)
  {
    case 1: return reinhard(x);
    case 2: return aces(x);
    case 3: return agx(x);
  }
  return no_tonemap(x);
}

// copypasted from: https://stackoverflow.com/questions/12964279/whats-the-origin-of-this-glsl-rand-one-liner
float rand(vec2 co)
{
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float signed_solid_angle(vec3 p, vec3 a, vec3 b, vec3 c)
{
	// Vectors from point to triangle vertices
	vec3 va = a - p;
	vec3 vb = b - p;
	vec3 vc = c - p;

	// Lengths
	float la = length(va);
	float lb = length(vb);
	float lc = length(vc);

	// Triple product (signed volume)
	float numerator = dot(va, cross(vb, vc));

	// Denominator
	float denominator =
		la * lb * lc +
		dot(va, vb) * lc +
		dot(vb, vc) * la +
		dot(vc, va) * lb;

	// atan2 gives correct sign and stability
	return 2.0 * atan(numerator, denominator);
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

  // This line
  {
    ivec2 mguv = ivec2(megatex_uv-vec2(0.5f));
    /*
    for (int i = -1; i <= 1; ++i)
      for (int j = -1; j <= 1; ++j)
        imageStore(megatex_mask, mguv + ivec2(i, j), vec4(1.0, 1.0, 1.0, 1.0));
    */
    imageStore(megatex_mask, mguv, vec4(1.0, 1.0, 1.0, 1.0));
  }
  
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
  // Hotfix for weird texture edges when mipmapping enabled..  This will break
  // with large textures!
  uv = clamp(uv, vec2(0.01f), vec2(0.99f));
  // compute atlas uv
  uv = (uv * texture_data.size + texture_data.pos) / atlas_size;

  vec4 color = texture(game_atlas_sampler, uv);
  //if (color.a < 0.95f)
  //  discard;

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
  vec4 megatex_value  = texelFetch(megatex_input,   ivec2(megatex_uv), 0);
  vec4 megatex_shadow = texelFetch(megatex_shadows, ivec2(megatex_uv), 0);
  vec4 debug_value    = texelFetch(megatex_debug,   ivec2(megatex_uv), 0);
  vec3 light = megatex_value.xyz * 1.0f + megatex_shadow.xyz * 1.0f;

  // Do the tonemapping here
  vec3 color_with_light = color.xyz * light;
  vec3 tonemapped = tonemap(color_with_light);

  g_albedo = vec4(tonemapped, 1.0f) + debug_value;
  g_sector = sector_id;
}
