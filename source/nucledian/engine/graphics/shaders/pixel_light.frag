constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core

in vec2 uv;
in vec3 wp;

layout(location = 3) uniform vec3 u_color;

out vec4 out_color;

void main()
{
  out_color = vec4(u_color, 1.0);
}

)";
