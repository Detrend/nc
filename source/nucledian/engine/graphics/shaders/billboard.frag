constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core
in vec3 position;
in vec2 uv;

out vec4 out_color;

uniform sampler2D sampler;

void main()
{
  out_color = texture(sampler, uv);
}

)";