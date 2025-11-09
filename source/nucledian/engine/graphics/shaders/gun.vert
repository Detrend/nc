constexpr const char* VERTEX_SOURCE = R"(

#version 430 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

out vec3 position;
out vec3 normal;
out vec2 uv;

layout(location = 0) uniform mat4 transform;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 projection;
layout(location = 3) uniform vec2 atlas_size;
layout(location = 4) uniform vec2 texture_pos;
layout(location = 5) uniform vec2 texture_size;

void main()
{
  gl_Position = projection * view * vec4(a_position, 1.0f);
  position = (transform * vec4(vec3(0.0f), 1.0f)).xyz;
  normal = (transform * vec4(0.0f, 0.0f, 1.0f, 0.0f)).xyz;
  uv = (a_uv * texture_size + texture_pos) / atlas_size;
}

)";