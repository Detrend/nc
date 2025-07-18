constexpr const char* VERTEX_SOURCE = R"(

#version 430 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

out vec2 uv;

void main()
{
  uv = a_uv;
  gl_Position = vec4(a_position, 1.0f);
}

)";