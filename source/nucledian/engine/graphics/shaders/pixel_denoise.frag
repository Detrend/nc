#version 430 core

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

layout(std430, binding = 0) readonly buffer parts_buffer   { MegatexPart megatex_parts[];   };

layout(binding = 0)        uniform sampler2D megatex;
layout(binding = 1)        uniform sampler2D megatex_mask;
layout(binding = 2, rgba8) uniform image2D   megatex_debug; // debug output
layout(location = 7)       uniform vec2      u_megatex_size;
layout(location = 9)       uniform uint      my_part_id;
layout(location = 10)      uniform ivec4     u_debug_part_pxx_pxy;
layout(location = 11)      uniform ivec4     u_horizontal;

out vec4      out_color;
flat in ivec2 from_px;
flat in ivec2 to_px;

// Indices we should consider
in flat uint  num_good_indices;
in flat uint  good_indices[6];
in flat vec2  good_offsets[6];
in flat vec2  good_scales[6];

#define GRID_N            10
#define DO_DENOISE        1
#define DO_PART_DEBUG     0
#define DO_DRAW_DEBUG     0
#define BLUR_ACROSS_EDGES 1
#define TONEMAPPER        1 // 0 = none, 1 = aces, 2 = reinhard

#if (TONEMAPPER == 1)
#define tonemap aces
#elif (TONEMAPPER == 2)
#define tonemap reinhard
#else
#define tonemap no_tonemap
#endif

#if DO_DRAW_DEBUG
void debug_sample(ivec2 pos, vec4 col)
{
  imageStore(megatex_debug, pos, col);
}
#define debug_store debug_sample
#else
void sample_nothing(ivec2 pos, vec4 col) {}
#define debug_store sample_nothing
#endif

#define PX_COL vec4(1.0f, 1.0f, 0.0f, 1.0f)
#define MY_COL vec4(1.0f, 0.0f, 0.0f, 1.0f)
#define SAMPLED_FROM_COL vec4(0.0f, 0.0f, 1.0f, 1.0f)

bool is_in_interval(ivec2 val, ivec2 from, ivec2 to)
{
  return clamp(val, from, to) == val;
}

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

void main()
{
  MegatexPart my_part = megatex_parts[my_part_id];

  ivec2 my_coord = ivec2(gl_FragCoord.xy);
  vec3  og_color = texture(megatex, my_coord / u_megatex_size).xyz;

  vec3  sum         = vec3(0.0f);
  float num_samples = 0.0f;

#if DO_PART_DEBUG
  if (my_part_id == u_debug_part_pxx_pxy.x)
  {
    ivec2 dbg_px = u_debug_part_pxx_pxy.yz;

    debug_store(ivec2(gl_FragCoord.xy), MY_COL);

    for (int i = 0; i < num_good_indices; ++i)
    {
      MegatexPart part = megatex_parts[good_indices[i]];
      for (int x = part.megatex_coord_1.x; x < part.megatex_coord_2.x; ++x)
      {
        for (int y = part.megatex_coord_1.y; y < part.megatex_coord_2.y; ++y)
        {
          if ((x + y) % 2 == 0)
          {
            debug_store(ivec2(x, y), SAMPLED_FROM_COL);
          }
        }
      }
    }

    // Aimed at pixel
    debug_store(dbg_px, PX_COL);
  }
#endif

#if DO_DENOISE
  float sigma_s = float(GRID_N-3);

  for (int k = -GRID_N; k <= GRID_N; ++k)
  {
    ivec2 true_coord = u_horizontal == 1
        ? ivec2(my_coord.x + k, my_coord.y)
        : ivec2(my_coord.x,     my_coord.y + k);
    ivec2 coord      = true_coord;

    ivec2 to_center     = abs(my_coord - true_coord);
    float center_dist_2 = dot(to_center, to_center);
    float w             = exp(-center_dist_2 / (2.0f * sigma_s * sigma_s));

    // Us - only in the range
    if (is_in_interval(coord, from_px, to_px-ivec2(1)))
    {
      vec3 mask_value = texture(megatex_mask, coord / u_megatex_size).xyz;
      if (mask_value.r > 0.0f)
      {
        vec3 neighbor_color = texture(megatex, coord / u_megatex_size).xyz;

        sum         += neighbor_color * w;
        num_samples += w;

        if (my_part_id == u_debug_part_pxx_pxy.x)
        {
          ivec2 dbg_px = u_debug_part_pxx_pxy.yz;
          if (dbg_px == my_coord)
          {
            debug_store(coord, vec4(0.0f, 1.0f, 0.0f, 1.0f) * w);
          }
        }
      }
    }

    // Others - everywhere
#if BLUR_ACROSS_EDGES
    for (int p = 0; p < num_good_indices; ++p)
    {
      MegatexPart part = megatex_parts[good_indices[p]];

      vec2 px_c  = true_coord;
      vec2 from  = ivec2(part.megatex_coord_1);
      vec2 to    = ivec2(part.megatex_coord_2);

      vec2 scale  = good_scales[p];
      vec2 offset = good_offsets[p];

      ivec2 other_part_coord_within = ivec2(scale * px_c + offset);

      if (is_in_interval(other_part_coord_within, ivec2(from), ivec2(to)-ivec2(1)))
      {
        vec3 mask_value = texture(megatex_mask, other_part_coord_within / u_megatex_size).xyz;
        if (mask_value.r > 0.0f)
        {
          vec3 neighbor_color = texelFetch(megatex, other_part_coord_within, 0).xyz;

          sum         += neighbor_color * w;
          num_samples += w;

          if (my_part_id == u_debug_part_pxx_pxy.x)
          {
            ivec2 dbg_px = u_debug_part_pxx_pxy.yz;
            if (dbg_px == my_coord)
            {
              debug_store(other_part_coord_within, vec4(0.0f, 1.0f, 1.0f, 1.0f) * w);
            }
          }
        }
      }
    }
#endif
  }

  vec3 averaged_color = sum / max(num_samples, 0.0001f);

  // Tonemap only for vertical pass
  if (u_horizontal.x != 0)
  {
    out_color = vec4(averaged_color, 1.0f);
  }
  else
  {
    out_color = vec4(tonemap(averaged_color), 1.0f);
  }
#else
  out_color = vec4(tonemap(og_color), 1.0f);
#endif
}
