#version 430 core

#define MAX_PARTS 512
#define BALANCE_SAMPLING_BY_SOLID_ANGLE 1
#define NUM_SAMPLES_PER_PIXEL_WHEN_NOT_BALANCING 6
#define NUM_SAMPLES_TOTAL_WHEN_BALANCING 128
#define DEBUG_DRAW 1
#define CULL_INVISIBLE_PIXELS 1

bool out_of_bounds = false;
bool out_of_parts  = false;
bool no_samples    = false;
bool not_visible   = false;

struct MegatexPart
{
  ivec2 megatex_coord_1;
  ivec2 megatex_coord_2;
  vec3  wpos_00;
  float texture_id;
  vec3  wpos_10;
  float texture_scale;
  vec3  wpos_01;
  float cumulative_wall_len_start;
  vec3  wpos_11;
  float texture_offset_x;
  vec3  normal;
  float texture_offset_y;
};

struct TextureData
{
  vec2  pos;
  vec2  size;
  float in_game_atlas;
};


in vec2 uv;
in vec3 wp;
in vec3 normal;

layout(location = 7)  uniform vec2 u_megatex_size;
layout(location = 8)  uniform uint num_indices;
layout(location = 9)  uniform uint my_part_id;
layout(location = 10) uniform vec2 game_atlas_size;

layout(std430, binding = 0) readonly buffer parts_buffer   { MegatexPart megatex_parts[];   };
layout(std430, binding = 1) readonly buffer indices_buffer { uint        megatex_indices[]; };
layout(std430, binding = 2) readonly buffer texture_buffer { TextureData textures[];        };

layout(binding = 0) uniform sampler2D megatex_input;
layout(binding = 1) uniform sampler2D game_atlas_sampler;
layout(binding = 2) uniform sampler2D megatex_mask;

out vec4 out_color;

// uv_coords is in range [0, 1]
// always samples from the game atlas
vec3 fetch_part_surface_texture(MegatexPart part, vec2 uv_coords)
{
  if (part.texture_id < 0.0f)
  {
    return vec3(1.0f, 0.0f, 1.0f);
  }

  TextureData texture_data = textures[int(part.texture_id)];

  // This is the world position of the pixel we are sampling
  vec3 wp_bottom    = mix(part.wpos_00, part.wpos_10, uv_coords.x);
  vec3 wp_top       = mix(part.wpos_01, part.wpos_11, uv_coords.x);
  vec3 wp_of_sample = mix(wp_bottom, wp_top, uv_coords.y);

  float cumulative_wall_len = part.cumulative_wall_len_start + distance(part.wpos_00.xz, part.wpos_10.xz) * uv_coords.x;
  vec2 tex_uv = abs(part.normal.y) > 0.99f ? wp_of_sample.xz : vec2(cumulative_wall_len, wp_of_sample.y);

  // floor uv is mirrored
  if (part.normal.y > 0.0f) tex_uv.x *= -1.0f;

  // NOTE: rotation skipped

  tex_uv = tex_uv / part.texture_scale + vec2(part.texture_offset_x, part.texture_offset_y);
  // tile texture
  tex_uv = fract(tex_uv);
  // flip y axis
  tex_uv.y = 1.0f - tex_uv.y;
  // Hotfix for weird texture edges when mipmapping enabled..  This will break
  // with large textures!
  tex_uv = clamp(tex_uv, vec2(0.01f), vec2(0.99f));
  // compute atlas uv
  tex_uv = (tex_uv * texture_data.size + texture_data.pos) / game_atlas_size;

  return texture(game_atlas_sampler, tex_uv).rgb;
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

float calc_part_signed_angle(MegatexPart part)
{
  float signed_angle = 0.0f;
  signed_angle += signed_solid_angle(wp, part.wpos_00, part.wpos_01, part.wpos_10);
  signed_angle += signed_solid_angle(wp, part.wpos_01, part.wpos_11, part.wpos_10);
  if (part.normal.y > 0.5)
  {
    // floor
    signed_angle *= -1.0f;
  }
  return signed_angle;
}

float rand(vec2 co)
{
  return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

uint hash(uint x)
{
  x ^= x >> 16;
  x *= 0x7feb352dU;
  x ^= x >> 15;
  x *= 0x846ca68bU;
  x ^= x >> 16;
  return x;
}

uint hash(ivec2 v)
{
  return hash(uint(v.x) * 0x1f1f1f1fU ^ uint(v.y));
}

ivec2 noise2(ivec2 p)
{
  uint h1 = hash(p);
  uint h2 = hash(p + ivec2(1, 0));

  return ivec2(max(int(h1), 0), max(int(h2), 0));
}

float triangleArea(vec3 a, vec3 b, vec3 c)
{
  return 0.5 * length(cross(b - a, c - a));
}

float quadArea(vec3 v0, vec3 v1, vec3 v2, vec3 v3)
{
  // Split quad into two triangles: (v0, v1, v2) and (v0, v2, v3)
  float area1 = triangleArea(v0, v1, v2);
  float area2 = triangleArea(v0, v2, v3);
  return area1 + area2;
}

void main()
{
  const float one_over_pi = 1.0 / 3.141692;
  const float max_signed_angle = 3.141592f * 2.0f;

  float angles_per_part[MAX_PARTS] = float[MAX_PARTS](0.0f);

  vec3 mask_value = texture(megatex_mask, gl_FragCoord.xy / u_megatex_size).xyz;
#if DEBUG_DRAW
  /*
  if (mask_value.r <= 0.0f)
  {
    not_visible = true;
  }
  */
#endif

#if CULL_INVISIBLE_PIXELS
  if (mask_value.r <= 0.0f)
  {
    out_color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    return;
  }
#endif

#if DEBUG_DRAW
  if (num_indices > MAX_PARTS)
  {
    out_of_parts = true;
  }
#endif

  // calculate samples per part
  float angle_sum = 0.0f;
  for (int i = 0; i < num_indices; ++i)
  {
    uint part_id = megatex_indices[i];
    if (part_id == my_part_id)
    {
      // Skip us
      continue;
    }

    // Sample random points on the surface of the object
    MegatexPart part = megatex_parts[part_id];

    if (part.texture_id < 0.0f)
    {
      continue;
    }

    vec3 part_n = part.normal;
    if (dot(-normal, part_n) < 0.0f)
    {
      continue;
    }

    float signed_angle = calc_part_signed_angle(part);
    if (signed_angle < 0.0f)
    {
      continue;
    }

    float solid_angle = signed_angle / max_signed_angle;
    angles_per_part[i] = solid_angle;
    angle_sum += solid_angle;
  }

  vec3 sum = vec3(0.0);
  int  num_samples_total = 0;

  vec3 og_color = texture(megatex_input, gl_FragCoord.xy / u_megatex_size).xyz;

  ivec2 frag_idx = ivec2(gl_FragCoord.xy);

  for (int i = 0; i < num_indices; ++i)
  {
    uint part_id = megatex_indices[i];
    if (part_id == my_part_id)
    {
      // Skip us
      continue;
    }

    // Sample random points on the surface of the object
    MegatexPart part = megatex_parts[part_id];

    if (part.texture_id < 0.0f)
    {
      continue;
    }

    vec3 part_n = part.normal;
    if (dot(-normal, part_n) < 0.0f)
    {
      continue;
    }

    // Always positive, good
    ivec2 size_px = part.megatex_coord_2 - part.megatex_coord_1 + ivec2(1);

    ivec2 from = part.megatex_coord_1;
    ivec2 to   = part.megatex_coord_2;

    vec3  part_sum  = vec3(0.0f);
    float part_area = quadArea(part.wpos_00, part.wpos_10, part.wpos_01, part.wpos_11);

#if BALANCE_SAMPLING_BY_SOLID_ANGLE
    float this_part_angle = angles_per_part[i];
    int num_samples_for_this_part = int((this_part_angle / angle_sum) * NUM_SAMPLES_TOTAL_WHEN_BALANCING);
#else
    angle_sum = 1.0f;
    float this_part_angle = 1.0f;
    int num_samples_for_this_part = NUM_SAMPLES_PER_PIXEL_WHEN_NOT_BALANCING;
#endif

#if DEBUG_DRAW
    /*
    if (num_samples_for_this_part > 0)
    {
      no_samples = true;
    }
    */
#endif

    for (uint sample_idx = 0; sample_idx < num_samples_for_this_part; ++sample_idx)
    {
      // Always positive, ok
      ivec2 coords_out_of_bounds = noise2(ivec2(part_id, sample_idx * 41) + frag_idx * ivec2(1001, 387));
      ivec2 cnt = coords_out_of_bounds / size_px;

      ivec2 vec2_coords = coords_out_of_bounds - cnt * size_px;

      vec2  uv_coords   = vec2(vec2_coords) / vec2(size_px);
      ivec2 true_coords = from + vec2_coords; // Sample this

      if (uv_coords.x < 0 || uv_coords.x > 1 || uv_coords.y < 0 || uv_coords.y > 1)
      {
        out_of_bounds = true;
      }

      vec3 smple = texelFetch(megatex_input, true_coords, 0).xyz;

      vec3 wp_bottom    = mix(part.wpos_00, part.wpos_10, uv_coords.x);
      vec3 wp_top       = mix(part.wpos_01, part.wpos_11, uv_coords.x);

      // This is the world position of the pixel we are sampling
      vec3 wp_of_sample = mix(wp_bottom, wp_top, uv_coords.y);

      // Now calculate the UV of the texture that is on the wall
      vec3 albedo = fetch_part_surface_texture(part, uv_coords);

      vec3  dir    = normalize(wp_of_sample - wp); // direction to the sample
      float angle1 = max(dot(normal, dir),  0.0f);  // angle 
      float angle2 = max(dot(part_n, -dir), 0.0f);
      float dist   = distance(wp_of_sample, wp);
      float attenuation = 1.0f / (dist * dist);

      part_sum += smple * albedo * attenuation * angle1 * angle2 * part_area * angle_sum / this_part_angle;
      num_samples_total += 1;
    }

    // average it out
    sum += part_sum;
  }

#if DEBUG_DRAW
#if BALANCE_SAMPLING_BY_SOLID_ANGLE
  if (num_samples_total > NUM_SAMPLES_TOTAL_WHEN_BALANCING)
  {
    out_of_bounds = true;
  }
#endif
#endif

  sum = sum * one_over_pi / num_samples_total;

  vec3 final_color = vec3(0.0, 0.0, 0.0);
#if DEBUG_DRAW
  if (false)
  {
    final_color = fetch_part_surface_texture(megatex_parts[my_part_id], uv);
  }
  else if (out_of_bounds)
  {
    final_color = vec3(1.0, 0.0, 0.0);
  }
  else if (no_samples)
  {
    final_color = vec3(0.0, 0.0, 1.0);
  }
  else if (out_of_parts)
  {
    final_color = vec3(1.0, 0.0, 1.0);
  }
  else if (not_visible)
  {
    final_color = vec3(0.0, 1.0, 0.0);
  }
  else
#endif
  {
    final_color = (og_color * 1.0f + sum * 1.0) * 1.0f;
  }
  out_color = vec4(final_color, 1.0f);
}
