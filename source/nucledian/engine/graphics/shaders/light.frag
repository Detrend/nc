constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core

struct DirLight
{
    vec3  color;
    float intensity;
    vec3  direction;
};

in vec2 uv;

out vec4 out_color;

layout(location = 0) uniform sampler2D g_position;
layout(location = 1) uniform sampler2D g_normal;
layout(location = 2) uniform sampler2D g_albedo;

layout(location = 3) uniform vec3 view_position;
layout(location = 4) uniform uint num_dir_lights;

layout(std430, binding = 0) buffer DirLights { DirLight dir_lights[]; };

void main()
{
  vec4 g_position_sample = texture(g_position, uv);
  vec3 position = g_position_sample.xyz;
  // 4-th component of position is used to determine if pixel should be lit
  bool unlit = g_position_sample.w == 0.0f;

  vec4 g_normal_sample = texture(g_normal, uv);
  vec3 normal = g_normal_sample.xyz;
  // 4-th component of normal is used for specular strength
  float specular_strength = g_normal_sample.w;

  vec3 albedo = texture(g_albedo, uv).rgb;

  if (unlit)
  {
    out_color = vec4(albedo, 1.0f);
    return;
  }

  float ambient_strength = 0.3f;
  int   shininess        = 128;

  vec3 view_direction = normalize(view_position - position);
  vec3 final_color    = ambient_strength * albedo;

  for(int i = 0; i < num_dir_lights; i++)
  {
    vec3 reflect_direction = reflect(-dir_lights[i].direction, normal);

    vec3 diffuse  = max(dot(normal, dir_lights[i].direction), 0.0f) * albedo;
    vec3 specular = specular_strength * pow(max(dot(view_direction, reflect_direction), 0.0f), shininess) * vec3(1.0f);

    final_color += (diffuse + specular) * dir_lights[i].color * dir_lights[i].intensity;
  }

  out_color = vec4(final_color, 1.0f);
}

)";