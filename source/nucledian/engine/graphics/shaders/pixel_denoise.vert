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

layout(location = 0) uniform vec2 u_from;
layout(location = 1) uniform vec2 u_to;
layout(location = 2) uniform vec3 u_wp00;
layout(location = 3) uniform vec3 u_wp10;
layout(location = 4) uniform vec3 u_wp01;
layout(location = 5) uniform vec3 u_wp11;
layout(location = 6) uniform vec3 u_norm;
layout(location = 7) uniform vec2 u_megatex_size;
layout(location = 8) uniform uint num_indices;
layout(location = 9) uniform uint my_part_id;

out vec2 uv;
out vec3 wp;
out vec3 normal;

flat out ivec2 from_px;
flat out ivec2 to_px;
out flat uint  num_good_indices;
out flat uint  good_indices[6];
out flat vec2  good_offsets[6];
out flat vec2  good_scales[6];

layout(std430, binding = 0) readonly buffer parts_buffer   { MegatexPart megatex_parts[];   };
layout(std430, binding = 1) readonly buffer indices_buffer { uint        megatex_indices[]; };

float det2(vec2 a, vec2 b)
{
  return a.x * b.y - a.y * b.x;
}

void main()
{
  // Build a quad from 6 vertices using gl_VertexID
  const vec2 corners[6] = vec2[](
    vec2(0, 0), vec2(1, 0), vec2(1, 1),
    vec2(0, 0), vec2(1, 1), vec2(0, 1)
  );

  from_px = ivec2(u_from);
  to_px   = ivec2(u_to);

  vec2 local = corners[gl_VertexID];
  vec2 pixel = mix(u_from, u_to, local);
  vec2 ndc   = (pixel / u_megatex_size) * 2.0 - 1.0;

  vec3 wp_bottom = mix(u_wp00, u_wp10, local.x);
  vec3 wp_top    = mix(u_wp01, u_wp11, local.x);
  wp             = mix(wp_bottom, wp_top, local.y);
  uv             = local;
  normal         = u_norm;
  gl_Position = vec4(ndc, 0.0, 1.0);

  // Calculate which other faces we want to include in denoising
  num_good_indices = 0;
  MegatexPart my_part = megatex_parts[my_part_id];
  bool i_am_wall = abs(my_part.normal.y) < 0.1f;
  for (int i = 0; i < num_indices && num_good_indices < 6; ++i)
  {
    uint index = megatex_indices[i];
    MegatexPart part = megatex_parts[index];

    if (index == my_part_id)
    {
      continue;
    }

    bool is_wall = abs(part.normal.y) < 0.1f;
    if (i_am_wall != is_wall)
    {
      continue;
    }

    if (is_wall)
    {
      vec2 my_p1 = my_part.wpos_00.xz;
      vec2 my_p2 = my_part.wpos_10.xz;
      vec2 it_p1 = part.wpos_00.xz;
      vec2 it_p2 = part.wpos_10.xz;

      bool a = distance(my_p1, it_p2) < 0.01f;
      bool b = distance(my_p2, it_p1) < 0.01f;

      vec2 dir1, dir2;

      if (a && b)
      {
        continue;
      }
      else if (a)
      {
        // my_p1 and it_p2 are same
        dir1 = my_p2 - my_p1;
        dir2 = it_p1 - it_p2;
      }
      else if (b)
      {
        // my_p2 and it_p1 are same
        dir1 = my_p2 - my_p1;
        dir2 = it_p2 - it_p1;
      }
      else
      {
        continue;
      }

      if (det2(dir1, dir2) < 0.0f)
      {
        continue;
      }

      vec2 my_from = vec2(my_part.megatex_coord_1);
      vec2 my_to   = vec2(my_part.megatex_coord_2);
      vec2 from    = vec2(part.megatex_coord_1);
      vec2 to      = vec2(part.megatex_coord_2);

      float L_mine  = length(my_part.wpos_10.xz - my_part.wpos_00.xz);
      float L_other = length(part.wpos_10.xz    - part.wpos_00.xz);
      float scale_x  = L_mine / L_other * (to.x - from.x) / (my_to.x - my_from.x);
      float offset_x = a ? (to.x   - my_from.x * scale_x)
                         : (from.x - my_to.x   * scale_x);

      float scale_y  = (my_part.wpos_01.y - my_part.wpos_00.y) / (my_to.y - my_from.y)
                     * (to.y - from.y) / (part.wpos_01.y - part.wpos_00.y);
      float offset_y = from.y + (my_part.wpos_00.y - part.wpos_00.y)
                     * (to.y - from.y) / (part.wpos_01.y - part.wpos_00.y)
                     - my_from.y * scale_y;

      good_indices[num_good_indices] = index;
      good_offsets[num_good_indices] = vec2(offset_x, offset_y);
      good_scales [num_good_indices] = vec2(scale_x,  scale_y);
      num_good_indices += 1;
    }
    else
    {
      bool is_ceil   = part.normal.y < 0.1f;
      bool i_am_ceil = my_part.normal.y < 0.1f;
      if (is_ceil != i_am_ceil)
      {
        continue;
      }
      
      float my_y = my_part.wpos_00.y;
      float it_y = part.wpos_00.y;
      if (abs(my_y - it_y) > 0.05f)
      {
        continue;
      }

      vec2 from    = ivec2(part.megatex_coord_1);
      vec2 to      = ivec2(part.megatex_coord_2);
      vec2 my_from = ivec2(my_part.megatex_coord_1);
      vec2 my_to   = ivec2(my_part.megatex_coord_2);

      vec2 scale  = (my_part.wpos_11.xz - my_part.wpos_00.xz) / (my_to - my_from)
                  * (to - from) / (part.wpos_11.xz - part.wpos_00.xz);
      vec2 offset = from + (my_part.wpos_00.xz - part.wpos_00.xz)
                  * (to - from) / (part.wpos_11.xz - part.wpos_00.xz) - my_from * scale;

      good_indices[num_good_indices] = index;
      good_offsets[num_good_indices] = offset;
      good_scales [num_good_indices] = scale;
      num_good_indices += 1;
    }
  }
}
