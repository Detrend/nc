constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core
in vec3 world_position;

layout(location = 0) out vec4 g_position;
layout(location = 1) out vec4 g_normal;
layout(location = 2) out vec3 g_stitched_normal;
layout(location = 3) out vec4 g_albedo;

layout(binding = 0) uniform sampler2D equirectangular_map;

layout(location = 2) uniform float exposure;
layout(location = 3) uniform bool  use_gamma_correction;

const vec2 inv_atan = vec2(0.1591, 0.3183);

vec2 sample_spherical(vec3 direction)
{
  vec2 uv = vec2(atan(direction.z, direction.x), asin(direction.y)) * inv_atan;
  uv += 0.5;

  uv.y = 1.0 - uv.y;

  return uv;
}

void main()
{
  vec3 direction = normalize(world_position);
  vec2 uv = sample_spherical(direction);
  vec3 color = texture(equirectangular_map, uv).rgb;

  color = vec3(1.0) - exp(-color * exposure);
  if (use_gamma_correction)
    color = pow(color, vec3(1.0 / 2.2));
  g_albedo = vec4(color, 1.0);

  g_position.xyz = world_position;
  // 4-th component of position is used for specular strength
  g_position.w = 0.0f;
  // 4-th component of normal is used to determine if pixel should be lit
  g_normal = vec4(0.0f, 0.0f, 0.0f, 0.0f);
  g_stitched_normal = vec3(0.0f, 0.0f, 0.0f);
}

)";