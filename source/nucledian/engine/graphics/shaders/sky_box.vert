constexpr const char* VERTEX_SOURCE = R"(

#version 430 core
layout(location = 0) in vec3 a_position;
layout(location = 0) in vec3 a_normal;

out vec3 world_position;

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform mat4 projection;

void main()
{
  world_position = a_position;

  mat4 view_no_translate = mat4(mat3(view));
  vec4 position = projection * view_no_translate * vec4(a_position, 1.0);

  gl_Position = position.xyww;
}

)";