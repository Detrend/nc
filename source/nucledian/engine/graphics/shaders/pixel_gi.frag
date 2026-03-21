#version 430 core

struct MegatexPart
{
  ivec2 megatex_coord_1;
  ivec2 megatex_coord_2;
  vec3  wpos_00;
  float padd0;
  vec3  wpos_10;
  float padd1;
  vec3  wpos_01;
  float padd2;
  vec3  wpos_11;
  float padd3;
  vec3  normal;
  float padd4;
};

in vec2 uv;
in vec3 wp;
in vec3 normal;

layout(location = 7) uniform vec2 u_megatex_size;
layout(location = 8) uniform uint num_indices;
layout(location = 9) uniform uint my_part_id;

layout(std430, binding = 0) readonly buffer parts_buffer   { MegatexPart megatex_parts[];   };
layout(std430, binding = 1) readonly buffer indices_buffer { uint        megatex_indices[]; };

layout(binding = 0) uniform sampler2D megatex_input;

out vec4 out_color;

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

bool out_of_bounds = false;

void main()
{
  const uint num_samples_from_each = 32;

  vec3 sum = vec3(0.0);

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

    // Always positive, good
    ivec2 size_px = part.megatex_coord_2 - part.megatex_coord_1 + ivec2(1);

    ivec2 from = part.megatex_coord_1;
    ivec2 to   = part.megatex_coord_2;

    vec3 sample_sum = vec3(0.0f);
    float weight = 0.0f;

    for (uint sample_idx = 0; sample_idx < num_samples_from_each; ++sample_idx)
    {
      // Always positive, ok
      ivec2 coords_out_of_bounds = noise2(ivec2(part_id, sample_idx * 41) + frag_idx * ivec2(1001, 387));
      ivec2 cnt = coords_out_of_bounds / size_px;

      ivec2 vec2_coords = coords_out_of_bounds - cnt * size_px;

      vec2  uv_coords   = vec2(vec2_coords) / vec2(size_px);
      ivec2 true_coords = from + vec2_coords; // Sample this

      if (uv_coords.x < 0 || uv_coords.x > 1.0f || uv_coords.y < 0.0f)
      {
        out_of_bounds = true;
      }

      vec3 smple = texelFetch(megatex_input, true_coords, 0).xyz;

      vec3 wp_bottom    = mix(part.wpos_00, part.wpos_10, uv_coords.x);
      vec3 wp_top       = mix(part.wpos_01, part.wpos_11, uv_coords.x);
      vec3 wp_of_sample = mix(wp_bottom, wp_top, uv_coords.y);

      vec3  dir   = normalize(wp_of_sample - wp);
      float angle = max(dot(normal, dir), 0.0f);
      float dist  = distance(wp_of_sample, wp);
      float range = max(smple.x, max(smple.y, smple.z)) * 3.1415;
      float coeff = max(range - dist, 0.0f) / range;

      sample_sum += smple * (max(range - dist, 0.0) / max(range, 0.00001)) * angle;
    }

    // average it out
    sample_sum /= float(num_samples_from_each);
    //sample_sum /= max(weight, 0.0001);

    sum += sample_sum;
  }

  vec3 final_color = vec3(0.0, 0.0, 0.0);
  if (out_of_bounds)
  {
    final_color = vec3(1.0, 0.0, 0.0);
  }
  else
  {
    final_color = (og_color * 1.0f + sum * 1.0) * 0.1f;
  }
  //vec3 final_color = og_color;
  out_color = vec4(final_color, 1.0f);
}
