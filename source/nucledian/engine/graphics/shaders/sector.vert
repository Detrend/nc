constexpr const char* VERTEX_SOURCE = R"(

#version 430 core
layout (location = 0) in vec3  a_position;
layout (location = 1) in vec3  a_normal;
layout (location = 2) in float a_cumulative_wall_len;
layout (location = 3) in float a_texture_id;
layout (location = 4) in float a_texture_scale;
layout (location = 5) in float a_texture_rotation;
layout (location = 6) in float a_tile_rotations_count;
layout (location = 7) in float a_tile_rotation_increment;
layout (location = 8) in vec2  a_texture_offset;

out vec3 position;
out vec3 normal;
out float cumulative_wall_len;
flat out int texture_id;
flat out float texture_scale;
flat out float texture_rotation;
flat out float tile_rotations_count;
flat out float tile_rotation_increment;
flat out vec2 texture_offset;

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform mat4 projection;

void main()
{
  gl_Position = projection * view * vec4(a_position, 1.0f);
  normal = a_normal;
  position = a_position;
  cumulative_wall_len = a_cumulative_wall_len;
  texture_id = int(a_texture_id);
  texture_scale = a_texture_scale;
  texture_rotation = a_texture_rotation;
  texture_offset = a_texture_offset;
  tile_rotations_count = a_tile_rotations_count;
  tile_rotation_increment = a_tile_rotation_increment;
}

)";