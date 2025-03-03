constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core
in vec3 normal;
in vec3 position;

out vec4 out_color;

layout(location = 3) uniform vec4 color;
layout(location = 4) uniform vec3 view_position;

void main()
{
  float ambient_strength  = 0.05f;
  float specular_strength = 0.9f;
  int   shininess         = 128;

  vec3  light_color       = vec3(1.0f, 1.0f, 1.0f);
  vec3  light_direction   = -normalize(vec3(-0.2f, -0.8f, -0.4f));

  vec3 view_direction    = normalize(view_position - position);
  vec3 reflect_direction = reflect(-light_direction, normalize(normal));

  float ambient  = ambient_strength;
  float diffuse  = max(dot(normal, light_direction), 0.0f);
  float specular = specular_strength * pow(max(dot(view_direction, reflect_direction), 0.0f), shininess);

  out_color = vec4((ambient + diffuse + specular) * light_color, 1.0f) * color;
}

)";