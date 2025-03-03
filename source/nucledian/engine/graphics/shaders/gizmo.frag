constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core
in vec3 normal;

out vec4 out_color;

layout(location = 3) uniform vec4 color;

void main()
{
  vec3 light_color     = vec3(1.0f, 1.0f, 1.0f);
  vec3 light_direction = normalize(-vec3(-0.2f, -0.8f, -0.2f));

  vec3 ambient = 0.5f * light_color;
  vec3 diffuse = max(dot(normal, light_direction), 0.0f) * light_color;

  out_color = vec4(ambient + diffuse, 1.0f) * color;
}

)";