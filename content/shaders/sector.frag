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
//layout(binding = 1) uniform sampler2D level_atlas_sampler;
layout(binding = 1) uniform sampler2D game_normal_sampler;
//layout(binding = 3) uniform sampler2D level_normal_sampler;
layout(binding = 2) uniform sampler2D game_specular_sampler;
//layout(binding = 5) uniform sampler2D level_specular_sampler;

layout(binding = 3) uniform sampler2D megatex_debug;
layout(binding = 4) uniform sampler2D megatex_input;
layout(binding = 5) uniform sampler2D megatex_shadows;

layout(location = 2) uniform vec2  game_atlas_size;
layout(location = 3) uniform vec2  level_atlas_size;
layout(location = 5) uniform uint  sector_id;
layout(location = 6) uniform uint  matrix_id;
layout(location = 7) uniform ivec4 u_tonemap_direct_indirect;

#define PIXELS_PER_M 48

layout(std430, binding = 0) buffer texture_buffer { TextureData textures[]; };

layout(binding = 6, r8) uniform image2D megatex_mask;

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
  switch (u_tonemap_direct_indirect.x)
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
  //bool use_game_atlas = texture_data.in_game_atlas == 1.0f;
  //vec2 atlas_size = (use_game_atlas ? game_atlas_size : level_atlas_size);
  vec2 atlas_size = game_atlas_size;

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
  //if (use_game_atlas) {
    color = texture(game_atlas_sampler, uv);
    normal_rgb = texture(game_normal_sampler, uv).xyz;
    specular_strength = texture(game_specular_sampler, uv).r;
  //} else{
  //  color = texture(level_atlas_sampler, uv);
  //  normal_rgb = texture(level_normal_sampler, uv).xyz;
  //  specular_strength = texture(level_specular_sampler, uv).r;
  //}
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
  vec4 megatex_value  = texelFetch(megatex_input,   ivec2(megatex_uv), 0);
  vec4 megatex_shadow = texelFetch(megatex_shadows, ivec2(megatex_uv), 0);
  vec4 debug_value    = texelFetch(megatex_debug,   ivec2(megatex_uv), 0);

  float direct_coeff   = u_tonemap_direct_indirect.y == 1 ? 1.0f : 0.0f;
  float indirect_coeff = u_tonemap_direct_indirect.z == 1 ? 1.0f : 0.0f;

  vec3 light = megatex_value.xyz * indirect_coeff + megatex_shadow.xyz * direct_coeff;

  // Do the tonemapping here
  //vec3 color_with_light = color.xyz * light;

  vec3 color_with_light = color.xyz * light;
  vec3 tonemapped = tonemap(color_with_light);

  g_albedo = vec4(mix(tonemapped, debug_value.xyz, debug_value.w), 1.0f);
  //g_albedo = vec4(uv, 1.0f, 1.0f);
  g_sector = sector_id;
}
