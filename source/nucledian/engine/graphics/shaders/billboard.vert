
#version 430 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

out vec3 position;
out vec3 stitched_position;
out vec3 normal;
out vec2 uv;

layout(location = 0) uniform mat4 transform;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 projection;
layout(location = 3) uniform vec2 atlas_size;
layout(location = 4) uniform vec2 texture_pos;
layout(location = 5) uniform vec2 texture_size;
layout(location = 7) uniform mat4 portal_dest_to_src;

void main()
{
  gl_Position = projection * view * transform * vec4(a_position, 1.0f);
  stitched_position = (portal_dest_to_src * transform * vec4(a_position, 1.0f)).xyz;
  position = (transform * vec4(a_position, 1.0f)).xyz;
  // NOTE: Not sure why it has to be -1.0f, but with 1.0f it is flipped
  normal = (transform * vec4(0.0f, 0.0f, -1.0f, 0.0f)).xyz;
  uv = (a_uv * texture_size + texture_pos) / atlas_size;
}
