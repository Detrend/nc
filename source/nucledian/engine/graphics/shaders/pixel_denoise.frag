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

out vec4  out_color;
flat in ivec2 from_px;
flat in ivec2 to_px;

// Indices we should consider
in flat uint  num_good_indices;
in flat uint  good_indices[6];
in flat ivec4 good_offsets[6];

#define GRID_N        10
#define DO_DENOISE    1
#define DO_PART_DEBUG 0
#define DO_DRAW_DEBUG 1

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
  for (int i = my_coord.x - GRID_N; i <= my_coord.x + GRID_N; ++i) // horizontal
  {
    for (int j = my_coord.y - GRID_N; j <= my_coord.y + GRID_N; ++j) // vertical
    {
      ivec2 true_coord = ivec2(i, j);

      vec3  this_pixel_avg = vec3(0.0f);
      float this_pixel_cnt = 0.0f;

      ivec2 to_center     = abs(my_coord - true_coord);
      float center_dist_2 = dot(to_center, to_center);

      vec3  color      = vec3(0.0f);
      float weight     = 0.0f;
      float sum_weight = 0.0f;

      // Us - only in the range
      ivec2 coord       = true_coord;

      if (is_in_interval(coord, from_px, to_px-ivec2(1)))
      {
        vec3 mask_value = texture(megatex_mask, coord / u_megatex_size).xyz;
        if (mask_value.r > 0.0f)
        {
          ivec2 d = abs(true_coord - my_coord);

          color      += texture(megatex, coord / u_megatex_size).xyz;
          sum_weight += 1.0f;

          if (my_part_id == u_debug_part_pxx_pxy.x)
          {
            ivec2 dbg_px = u_debug_part_pxx_pxy.yz;
            if (dbg_px == my_coord)
            {
              debug_store(coord, vec4(0.0f, 1.0f, 0.0f, 1.0f));
            }
          }
        }
      }

      // Others - everywhere
      for (int p = 0; p < num_good_indices; ++p)
      {
        MegatexPart part = megatex_parts[good_indices[p]];

        vec2 px_c    = true_coord;
        vec2 from    = ivec2(part.megatex_coord_1);
        vec2 to      = ivec2(part.megatex_coord_2);
        vec2 my_from = ivec2(my_part.megatex_coord_1);
        vec2 my_to   = ivec2(my_part.megatex_coord_2);

        // input:  px_c = pixel coords of me
        // output: pixel_coords within other

        vec2  our_px_rel  = (px_c - my_from) / (my_to - my_from);
        vec2  our_wpos    = mix(my_part.wpos_00.xz, my_part.wpos_11.xz, our_px_rel);
        vec2  other_rel_p = (our_wpos - part.wpos_00.xz) / (part.wpos_11.xz - part.wpos_00.xz);
        ivec2 other_part_coord_within = ivec2(mix(part.megatex_coord_1, part.megatex_coord_2, other_rel_p));

        if (is_in_interval(other_part_coord_within, ivec2(from), ivec2(to)-ivec2(1)))
        {
          if (my_part_id == u_debug_part_pxx_pxy.x)
          {
            ivec2 dbg_px = u_debug_part_pxx_pxy.yz;
            if (dbg_px == my_coord)
            {
              debug_store(other_part_coord_within, vec4(0.0f, 1.0f, 1.0f, 1.0f));
            }
          }
        }
      }

      sum += color * weight;
      num_samples += weight;
    }
  }

  out_color = vec4(sum / max(num_samples, 0.0001f), 1.0f);
#else
  out_color = vec4(og_color, 1.0f);
#endif
}
