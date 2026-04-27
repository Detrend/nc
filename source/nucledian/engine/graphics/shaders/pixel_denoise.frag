#version 430 core

layout(location = 7) uniform vec2 u_megatex_size;

layout(binding = 0) uniform sampler2D megatex;
layout(binding = 1) uniform sampler2D megatex_mask;

out vec4  out_color;
flat in ivec2 from_px;
flat in ivec2 to_px;

#define GRID_N 10
#define DO_DENOISE 1

void main()
{
  ivec2 my_coord = ivec2(gl_FragCoord.xy);
  vec3  og_color = texture(megatex, my_coord / u_megatex_size).xyz;

  vec3  sum         = vec3(0.0f);
  float num_samples = 0.0f;

#if DO_DENOISE
  for (int i = max(my_coord.x - GRID_N, from_px.x); i <= min(my_coord.x + GRID_N, to_px.x); ++i) // horizontal
  {
    for (int j = max(my_coord.y - GRID_N, from_px.y); j <= min(my_coord.y + GRID_N, to_px.y); ++j) // vertical
    {
      ivec2 coord  = ivec2(i, j);
      float weight = sqrt(GRID_N * GRID_N) - distance(coord, my_coord) + 1.0f;
      vec3  color = texture(megatex, coord / u_megatex_size).xyz;
      sum += color * weight;
      num_samples += weight;
    }
  }

  out_color = vec4(sum / num_samples, 1.0f);
#else
  out_color = vec4(og_color, 1.0f);
#endif
}
