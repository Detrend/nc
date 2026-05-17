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

layout(binding = 0)  uniform sampler2D megatex;
layout(binding = 1)  uniform sampler2D megatex_mask;
layout(binding = 2, rgba8) uniform image2D megatex_debug; // debug output

layout(location = 7) uniform vec2 u_megatex_size;

layout(location = 9) uniform uint my_part_id;

out vec4  out_color;
flat in ivec2 from_px;
flat in ivec2 to_px;

// Indices we should consider
in flat uint  num_good_indices;
in flat uint  good_indices[6];
in flat ivec4 good_offsets[6];

#define GRID_N 10
#define DO_DENOISE 1
#define DEBUG_PART 0
//#define DEBUG_PART 6

#define MY_COL vec4(1.0f, 0.0f, 0.0f, 1.0f)
#define SAMPLED_FROM_COL vec4(0.0f, 0.0f, 1.0f, 1.0f)

void main()
{
  ivec2 my_coord = ivec2(gl_FragCoord.xy);
  vec3  og_color = texture(megatex, my_coord / u_megatex_size).xyz;

  vec3  sum         = vec3(0.0f);
  float num_samples = 0.0f;

#if DEBUG_PART
  if (my_part_id == DEBUG_PART)
  {
    imageStore(megatex_debug, ivec2(gl_FragCoord.xy), MY_COL);

    for (int i = 0; i < num_good_indices; ++i)
    {
      MegatexPart part = megatex_parts[good_indices[i]];
      for (int x = part.megatex_coord_1.x; x < part.megatex_coord_2.x; ++x)
      {
        for (int y = part.megatex_coord_1.y; y < part.megatex_coord_2.y; ++y)
        {
          if ((x + y) % 2 == 0)
          {
            imageStore(megatex_debug, ivec2(x, y), SAMPLED_FROM_COL);
          }
        }
      }
    }
  }
#endif

#if DO_DENOISE
  //for (int i = max(my_coord.x - GRID_N, from_px.x); i <= min(my_coord.x + GRID_N, to_px.x-1); ++i) // horizontal
  for (int i = max(my_coord.x - GRID_N, 0); i <= min(my_coord.x + GRID_N, int(u_megatex_size.x)-1); ++i) // horizontal
  {
    //for (int j = max(my_coord.y - GRID_N, from_px.y); j <= min(my_coord.y + GRID_N, to_px.y-1); ++j) // vertical
    for (int j = max(my_coord.y - GRID_N, 0); j <= min(my_coord.y + GRID_N, int(u_megatex_size.y)-1); ++j) // vertical
    {
      vec3  color      = vec3(0.0f);
      ivec2 true_coord = ivec2(i, j);
      float weight = 0.0f;

      // Us - only in the range
      {
        ivec2 coord = true_coord;

        if (clamp(coord, from_px, to_px-ivec2(1)) == coord)
        {
          vec3 mask_value = texture(megatex_mask, coord / u_megatex_size).xyz;
          if (mask_value.r <= 0.0f)
          {
            continue;
          }

          ivec2 d = abs(true_coord - my_coord);

          color  += texture(megatex, coord / u_megatex_size).xyz;
          //weight += 2 * GRID_N - float(d.x + d.y);
          weight += 1.0f;
        }
      }

      // Others - everywhere
      for (int p = 0; p < num_good_indices; ++p)
      {
        ivec2 from = good_offsets[p].xy;
        ivec2 to   = good_offsets[p].zw;

        MegatexPart part = megatex_parts[good_indices[p]];

        if (clamp(true_coord, from, to) == true_coord) // TODO: the "to" might be wrong here, possibly subtract 1
        {
          /*
          vec2  perc  = vec2(from - true_coord) / vec2(to - from);
          ivec2 coord = ivec2(mix(vec2(part.megatex_coord_1), vec2(part.megatex_coord_2), perc));

          vec3 mask_value = texture(megatex_mask, coord / u_megatex_size).xyz;
          if (mask_value.r <= 0.0f)
          {
            continue;
          }

          ivec2 d = abs(true_coord - my_coord);

          color  += texture(megatex, coord / u_megatex_size).xyz;
          //weight += 2 * GRID_N - float(d.x + d.y);
          weight += 1.0f;
          */
          imageStore(megatex_debug, ivec2(gl_FragCoord.xy), MY_COL);
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
