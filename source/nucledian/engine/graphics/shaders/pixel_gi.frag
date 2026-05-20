#version 430 core

#define MAX_PARTS 27
#define NUM_SAMPLES_TOTAL_WHEN_BALANCING 256
#define DEBUG_DRAW 0
#define CULL_INVISIBLE_PIXELS 0
#define INSPECTED_PART 0
#define HIGHLIGHT_THOSE_WHO_SAMPLE_FROM_THIS_PART 0
#define HIGHLIGHT_THOSE_SAMPLED_FROM_THIS_PART 0
#define DO_VISIBLITY_CHECK 1

#define HIGHLIGHT_SAMPLING_PX_MODE   1
#define HIGHLIGHT_SAMPLING_WALL_MODE 2
#define VISUALIZE_PARTS_MODE         3

// Possible improvements:
// [x] Debug that shows from where we sample.. Effectively another megatex.
// [x] Fix the number of samples we give to the parts.. Some parts receive 0
// [ ] Importance sampling based on the angle to the object as well.
//     For now, we do importance sampling based only on the solid angle size,
//     which is good, but giving more importance to objects with normals facing
//     us will definitely help.
// [ ] Sample triangles/quads uniformly with regards to their projection onto
//     POV - arvo sampling?
// [x] Parts that do not have any textures (empty walls for example) get added
//     to the pipeline.. Fix this.

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
  uint  sector_id;
  uint  unused_a;
  uint  unused_b;
  uint  unused_c;
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

struct TextureData
{
  vec2  pos;
  vec2  size;
  float in_game_atlas;
};

in smooth vec2 uv;
in smooth vec3 wp;
in smooth vec3 normal;

in flat uint num_good_indices;
in flat uint good_indices[26];

layout(location = 7)  uniform vec2  u_megatex_size;
layout(location = 8)  uniform uint  num_indices;
layout(location = 9)  uniform uint  my_part_id;
layout(location = 10) uniform vec2  game_atlas_size;
layout(location = 11) uniform uint  num_sectors;
layout(location = 12) uniform uint  num_walls;
layout(location = 13) uniform ivec4 dbg_part_pxx_pxy_mode;

layout(std430, binding = 0) readonly buffer parts_buffer       { MegatexPart megatex_parts[];   };
layout(std430, binding = 1) readonly buffer indices_buffer     { uint        megatex_indices[]; };
layout(std430, binding = 2) readonly buffer texture_buffer     { TextureData textures[];        };
layout(std430, binding = 3) readonly buffer sector_data_buffer { SectorData  sectors[];          };
layout(std430, binding = 4) readonly buffer wall_data_buffer   { WallData    walls[];            };

layout(binding = 0) uniform sampler2D megatex_input;
layout(binding = 1) uniform sampler2D game_atlas_sampler;
layout(binding = 2) uniform sampler2D megatex_mask;

layout(binding = 3, rgba8) uniform image2D megatex_debug;

float cross2(vec2 a, vec2 b)
{
  return a.x * b.y - a.y * b.x;
}

bool out_of_range_sectors = false;
bool out_of_range_walls   = false;
bool invalid_wall_id      = false;
bool max_hops_reached     = false;

// check intersection between top-down shadow ray and "wall ray"
// return t which tells at which point intersersection occurs
// if no intersection, returns -1.0f
float get_intersection_t(vec2 ray_origin, vec2 ray_direction, vec2 wall_p0, vec2 wall_p1)
{
  vec2 wall_origin = wall_p0;
  float wall_length = distance(wall_p1, wall_p0);
  vec2 wall_direction = normalize(wall_p1 - wall_p0);

  float denominator = cross2(ray_direction, wall_direction);

  // rays are parallel
  if (abs(denominator) < 0.001f)
    return -1.0f;

  vec2 diff = wall_origin - ray_origin;
  float inv_denominator = 1.0f / denominator;

  float ray_t  = cross2(diff, wall_direction) * inv_denominator;
  float wall_t = cross2(diff, ray_direction ) * inv_denominator;

  // intersection occurs outside of this wall segment
  if (wall_t < -0.001f || wall_t > wall_length + 0.001f)
    return -1.0f;

  return ray_t;
}

float shadow_coeff(vec3 from_position, vec3 to_position, uint start_sector_id, uint end_sector_id)
{
  if (start_sector_id == end_sector_id)
  {
    return 1.0f;
  }

  const uint INVALID_WALL_ID = 65535;
  const uint MAX_LIGHT_TRAVERSE_SECTORS = 32;

  vec3 ray_origin    = from_position;
  vec3 ray_direction = normalize(to_position - from_position);
  
  uint current_sector_id = start_sector_id;
  float ray_t = -0.001f;

  for (int hop = 0; hop < MAX_LIGHT_TRAVERSE_SECTORS; ++hop)
  {
    SectorData sector = sectors[current_sector_id];
    uint wall_index = INVALID_WALL_ID;

    if (current_sector_id == end_sector_id)
    {
      // check if ray collides with floor or ceiling
      float ray_hit_y = ray_origin.y + ray_t * ray_direction.y;
      if (ray_hit_y < sector.floor_y - 0.001f || ray_hit_y > sector.ceil_y + 0.001f)
      {
        return 0.0f;
      }

      return 1.0f;
    }

    if (current_sector_id >= num_sectors)
    {
      out_of_range_sectors = true;
      return 0.0f;
    }

    for (uint i = 0; i < sector.walls_count; ++i)
    {
      if (sector.walls_offset + i >= num_walls)
      {
        out_of_range_walls = true;
        return 0.0f;
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
      return 0.0f;
    }

    // check if ray collides with floor or ceiling
    float ray_hit_y = ray_origin.y + ray_t * ray_direction.y;
    if (ray_hit_y < sector.floor_y - 0.001f || ray_hit_y > sector.ceil_y + 0.001f)
    {
      return 0.0f;
    }
   
    WallData wall = walls[sector.walls_offset + wall_index];

    // wall is a solid wall (not a portal)
    if (wall.destination == INVALID_WALL_ID)
    {
      return 0.0f;
    }

    current_sector_id = wall.destination;

    /*
    mat4 portal_matrix = inverse(portal_matricies[wall.portal_matrix_index]);
    ray_origin = (portal_matrix * vec4(ray_origin, 1.0f)).xyz;
    ray_direction = normalize(mat3(portal_matrix) * ray_direction);
    */
  }

  max_hops_reached = true;
  return 0.0f;
}

vec3 uint_to_color(uint x)
{
  // PCG-style integer hash
  x ^= x >> 17;
  x *= 0xed5ad4bbu;
  x ^= x >> 11;
  x *= 0xac4c1b51u;
  x ^= x >> 15;
  x *= 0x31848babu;
  x ^= x >> 14;

  // Generate 3 channels from shuffled bits
  uint r = x;
  uint g = x * 1664525u + 1013904223u;
  uint b = x * 22695477u + 1u;

  // Convert to [0,1]
  return vec3(
    float(r & 0x00FFFFFFu),
    float(g & 0x00FFFFFFu),
    float(b & 0x00FFFFFFu)
  ) / float(0x00FFFFFFu);
}

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
	return 2.0 * atan(numerator, denominator); // this consumes about 4FPS
}

float calc_part_signed_angle(MegatexPart part)
{
  float signed_angle = 0.0f;
  signed_angle += signed_solid_angle(wp, part.wpos_00, part.wpos_01, part.wpos_10);
  signed_angle += signed_solid_angle(wp, part.wpos_01, part.wpos_11, part.wpos_10);
  float mul_by = float(part.normal.y < 0.5f) * 2.0f - 1.0f;
  signed_angle *= mul_by;
  return signed_angle;
}

// Assigns importance number to each part. Then we allocate the number of samples
// proportionally to the importance of each part.
// Approximately 30FPS can be gained by optimizing the shit out of this..
float calc_part_importance(uint part_id)
{
  MegatexPart part = megatex_parts[part_id];

  vec3 part_n = part.normal;

  float signed_angle = calc_part_signed_angle(part);
  if (signed_angle < 0.0f)
  {
    return 0.0f;
  }

  vec3 wp_of_part = (part.wpos_00 + part.wpos_10 + part.wpos_01 + part.wpos_11) * 0.25f;

  vec3  dir    = normalize(wp_of_part - wp); // direction to the sample
  float angle1 = max(dot(normal, dir),  0.0f);  // angle 
  float angle2 = max(dot(part_n, -dir), 0.0f);

  return signed_angle * angle1 * angle2;
  //return signed_angle;
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

float hash_3d(ivec3 p)
{
  uint x = uint(p.x);
  uint y = uint(p.y);
  uint z = uint(p.z);

  uint h = x * 0x8da6b343u ^ y * 0xd8163841u ^ z * 0xcb1ab31fu;

  h ^= (h >> 13);
  h *= 0x85ebca6bu;
  h ^= (h >> 16);

  return float(h) / float(0xffffffffu);
}

float triangle_area(vec3 a, vec3 b, vec3 c)
{
  return 0.5 * length(cross(b - a, c - a));
}

float quad_area(vec3 v0, vec3 v1, vec3 v2, vec3 v3)
{
  // Split quad into two triangles: (v0, v1, v2) and (v0, v2, v3)
  float area1 = triangle_area(v0, v1, v2);
  float area2 = triangle_area(v0, v2, v3);
  return area1 + area2;
}

vec2 hash_4d_to_2d(ivec4 p)
{
  uvec4 u = uvec4(p);

  uint h1 = u.x * 0x8da6b343u ^ u.y * 0xd8163841u ^ u.z * 0xcb1ab31fu ^ u.w * 0x165667b1u;
  uint h2 = u.x * 0xa24baedfu ^ u.y * 0x9fb21c65u ^ u.z * 0x5be0cd19u ^ u.w * 0x27d4eb2fu;

  // mix h1
  h1 ^= (h1 >> 16);
  h1 *= 0x7feb352du;
  h1 ^= (h1 >> 15);
  h1 *= 0x846ca68bu;
  h1 ^= (h1 >> 16);

  // mix h2
  h2 ^= (h2 >> 15);
  h2 *= 0x2c1b3c6du;
  h2 ^= (h2 >> 12);
  h2 *= 0x297a2d39u;
  h2 ^= (h2 >> 15);

  return vec2(
    float(h1) / float(0xffffffffu),
    float(h2) / float(0xffffffffu)
  );
}

// input:
  // sample_idx
  // part_id
// output:
  // wp_of_sample
  // albedo
  // sample
void sample_from_part(int sample_idx, uint part_id, out vec3 wp_of_sample, out vec3 albedo, out vec3 smple)
{
  // Sample random points on the surface of the object
  MegatexPart my_part = megatex_parts[my_part_id];
  MegatexPart part    = megatex_parts[part_id];
  ivec2 frag_idx = ivec2(gl_FragCoord.xy);

  // Always positive, good
  ivec2 size_px = part.megatex_coord_2 - part.megatex_coord_1 + ivec2(1);

  ivec2 from = part.megatex_coord_1;

  ivec4 seed        = ivec4(ivec2(part_id, sample_idx * 41), frag_idx * ivec2(1001, 387));
  vec2  uv_coords   = hash_4d_to_2d(seed);
  ivec2 true_coords = from + ivec2(uv_coords * size_px);

  if (uv_coords.x < 0 || uv_coords.x > 1 || uv_coords.y < 0 || uv_coords.y > 1)
  {
    out_of_bounds = true;
  }

  smple = texelFetch(megatex_input, true_coords, 0).xyz;

  if (dbg_part_pxx_pxy_mode.w == HIGHLIGHT_SAMPLING_PX_MODE)
  {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    int p    = dbg_part_pxx_pxy_mode.x;
    int px_x = dbg_part_pxx_pxy_mode.y;
    int px_y = dbg_part_pxx_pxy_mode.z;
    if (my_part_id == p && coord.x == px_x && coord.y == px_y)
    {
      imageStore(megatex_debug, ivec2(true_coords), vec4(0.0, 0.0, 1.0, 1.0));
    }
  }

  vec3 wp_bottom = mix(part.wpos_00, part.wpos_10, uv_coords.x);
  vec3 wp_top    = mix(part.wpos_01, part.wpos_11, uv_coords.x);

  // This is the world position of the pixel we are sampling
  wp_of_sample = mix(wp_bottom, wp_top, uv_coords.y);

  float occluded = 1.0f;
#if DO_VISIBLITY_CHECK
  occluded = shadow_coeff(wp, wp_of_sample, my_part.sector_id, part.sector_id);
#endif

  // Now calculate the UV of the texture that is on the wall
  albedo = fetch_part_surface_texture(part, uv_coords) * occluded;
}

void main()
{
  const float one_over_pi = 1.0 / 3.141692;
  ivec2 coords = ivec2(gl_FragCoord.xy);

  int dbg_mode = dbg_part_pxx_pxy_mode.w;
  if (dbg_mode == HIGHLIGHT_SAMPLING_WALL_MODE || dbg_mode == HIGHLIGHT_SAMPLING_PX_MODE)
  {
    int part_selected = dbg_part_pxx_pxy_mode.x;
    if (my_part_id == part_selected)
    {
      if ((coords.x & 1) == (coords.y & 1))
        imageStore(megatex_debug, ivec2(gl_FragCoord.xy), vec4(1.0, 0.0, 0.0, 1.0));
    }
  }
  else if (dbg_mode == VISUALIZE_PARTS_MODE)
  {
    // Visualize parts
    vec3 part_color = uint_to_color(my_part_id);
    if ((coords.x & 1) == (coords.y & 1))
    {
      part_color = vec3(0.0f);
    }

    imageStore(megatex_debug, coords, vec4(part_color, 1.0));
    out_color = vec4(1.0f);
    return;
  }

  if (dbg_part_pxx_pxy_mode.w == HIGHLIGHT_SAMPLING_PX_MODE)
  {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    int p    = dbg_part_pxx_pxy_mode.x;
    int px_x = dbg_part_pxx_pxy_mode.y;
    int px_y = dbg_part_pxx_pxy_mode.z;
    if (my_part_id == p && coord.x == px_x && coord.y == px_y)
    {
      imageStore(megatex_debug, coord, vec4(1.0, 1.0, 0.0, 1.0));
    }
  }

  float importance_per_part[MAX_PARTS] = float[MAX_PARTS](0.0f);

#if DEBUG_DRAW
  if (num_indices > MAX_PARTS)
  {
    out_of_parts = true;
  }
#endif

  // calculate samples per part
  float importance_sum = 0.0f;
  for (int i = 0; i < num_good_indices; ++i)
  {
    float importance = calc_part_importance(good_indices[i]);
    importance_per_part[i] = importance;
    importance_sum += importance;
    if (dbg_mode == HIGHLIGHT_SAMPLING_WALL_MODE)
    {
      if (my_part_id == dbg_part_pxx_pxy_mode.x)
      {
        MegatexPart part = megatex_parts[good_indices[i]];
        for (int x = part.megatex_coord_1.x; x < part.megatex_coord_2.x; ++x)
          for (int y = part.megatex_coord_1.y; y < part.megatex_coord_2.y; ++y)
            if (mod(x + y, 7) == 0)
              imageStore(megatex_debug, ivec2(x, y), vec4(0.0, 0.0, 1.0, 0.5));
      }
    }
  }

  vec3 sum = vec3(0.0);
  int  num_samples_total = 0;

  vec3 og_color = texture(megatex_input, gl_FragCoord.xy / u_megatex_size).xyz;

  ivec2 frag_idx = ivec2(gl_FragCoord.xy);

  for (int i = 0; i < num_good_indices; ++i)
  {
    uint part_id = good_indices[i];

    MegatexPart part = megatex_parts[part_id];

    vec3  part_sum  = vec3(0.0f);
    float part_area = quad_area(part.wpos_00, part.wpos_10, part.wpos_01, part.wpos_11);

    float this_part_importance      = importance_per_part[i];
    float importance_coeff          = this_part_importance / importance_sum;
    float num_samples_floating      = importance_coeff * NUM_SAMPLES_TOTAL_WHEN_BALANCING;
    int   num_samples_for_this_part = int(num_samples_floating);
    float rem = fract(num_samples_floating);
    if (hash_3d(ivec3(ivec2(gl_FragCoord.xy), i)) < rem)
    {
      num_samples_for_this_part += 1;
    }

    for (int sample_idx = 0; sample_idx < num_samples_for_this_part; ++sample_idx)
    {
      // Sample the part
      vec3 albedo;
      vec3 wp_of_sample;
      vec3 smple;
      sample_from_part(sample_idx, part_id, wp_of_sample, albedo, smple);

      vec3  dir    = normalize(wp_of_sample - wp); // direction to the sample
      float angle1 = max(dot(normal, dir),  0.0f);  // angle 
      float angle2 = max(dot(part.normal, -dir), 0.0f);
      float dist   = distance(wp_of_sample, wp);
      float attenuation = 1.0f / (dist * dist);

      part_sum += smple * albedo * attenuation * angle1 * angle2 * part_area * importance_sum / this_part_importance;
      num_samples_total += 1;
    }

    // average it out
    sum += part_sum;
  }

#if DEBUG_DRAW
  if (num_samples_total > NUM_SAMPLES_TOTAL_WHEN_BALANCING * 1.5f)
  {
    out_of_bounds = true;
  }
#endif

  //sum = sum * one_over_pi / max(num_samples_total, 1); // prevent division by 0
  // NOTE: Removed division by PI for consistency because we are already not doing it in the
  //       direct illumination shader.
  sum = sum / max(num_samples_total, 1); // prevent division by 0

  vec3 final_color = vec3(0.0, 0.0, 0.0);
#if DEBUG_DRAW
  if (false)
  {
    final_color = fetch_part_surface_texture(megatex_parts[my_part_id], uv);
  }
  else if (out_of_bounds)
  {
    final_color = vec3(0.0, 1.0, 1.0); // cyan
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
    final_color = (og_color * 0.0f + sum * 1.0) * 1.0f;
  }

  out_color = vec4(final_color, 1.0f);
}
