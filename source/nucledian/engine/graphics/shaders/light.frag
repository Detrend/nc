constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core
in vec2 uv;

out vec4 out_color;

layout(location = 0) uniform sampler2D g_position;
layout(location = 1) uniform sampler2D g_normal;
layout(location = 2) uniform sampler2D g_albedo;

layout(location = 3) uniform vec3 view_position;

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

  vec4 color = texture(g_albedo, uv);

  if (unlit)
  {
    out_color = color;
    return;
  }

  float ambient_strength = 0.3f;
  int   shininess         = 128;

  vec3  light_color       = vec3(1.0f, 1.0f, 1.0f);
  vec3  light_direction   = -normalize(vec3(-0.2f, -0.8f, -0.4f));

  vec3 view_direction    = normalize(view_position - position);
  vec3 reflect_direction = reflect(-light_direction, normal);

  float ambient  = ambient_strength;
  float diffuse  = max(dot(normal, light_direction), 0.0f);
  float specular = specular_strength * pow(max(dot(view_direction, reflect_direction), 0.0f), shininess);

  out_color = vec4((ambient + diffuse + specular) * light_color, 1.0f) * color;
}

)";