#version 430 core

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

layout(std430, binding = 0) readonly buffer parts_buffer   { MegatexPart megatex_parts[];   };
layout(std430, binding = 1) readonly buffer indices_buffer { uint        megatex_indices[]; };

out smooth vec2 uv;
out smooth vec3 wp;
out smooth vec3 normal;

out flat uint num_good_indices;
out flat uint good_indices[26];

// Returns true if part2 is behind the part1
bool is_part_behind_the_other(MegatexPart part1, MegatexPart part2)
{
  // Check if the part is behind us..
  vec3  p   = part1.wpos_00;
  vec3  a   = part2.wpos_00 - p;
  vec3  b   = part2.wpos_01 - p;
  vec3  c   = part2.wpos_10 - p;
  vec3  d   = part2.wpos_11 - p;
  float d_a = dot(part1.normal, a);
  float d_b = dot(part1.normal, b);
  float d_c = dot(part1.normal, c);
  float d_d = dot(part1.normal, d);

  return d_a <= 0.0f && d_b <= 0.0f && d_c <= 0.0f && d_d <= 0.0f;
}

void main()
{
  // Build a quad from 6 vertices using gl_VertexID
  const vec2 corners[6] = vec2[](
    vec2(0, 0), vec2(1, 0), vec2(1, 1),
    vec2(0, 0), vec2(1, 1), vec2(0, 1)
  );

  vec2 local = corners[gl_VertexID];
  vec2 pixel = mix(u_from, u_to, local);
  vec2 ndc   = (pixel / u_megatex_size) * 2.0 - 1.0;

  vec3 wp_bottom = mix(u_wp00, u_wp10, local.x);
  vec3 wp_top    = mix(u_wp01, u_wp11, local.x);
  wp             = mix(wp_bottom, wp_top, local.y);
  uv             = local;
  normal         = u_norm;
  gl_Position    = vec4(ndc, 0.0, 1.0);

  // Filter out the parts here in the vertex shader
  num_good_indices = 0;
  MegatexPart us = megatex_parts[my_part_id];
  for (uint i = 0; i < num_indices && num_good_indices < 26; ++i)
  {
    uint part_id = megatex_indices[i];

    if (part_id == my_part_id)
    {
      // This is us, do not sample ourselves
      continue;
    }

    MegatexPart part = megatex_parts[part_id];
    if (part.texture_id < 0)
    {
      // Does not have any textures, can happen?
      continue;
    }

    if (is_part_behind_the_other(us, part))
    {
      continue;
    }

    if (is_part_behind_the_other(part, us))
    {
      continue;
    }

    good_indices[num_good_indices++] = part_id;
  }
}
