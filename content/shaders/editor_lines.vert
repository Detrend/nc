#version 430 core

// Input
layout(location = 0) in vec2 a_position;

// Uniforms
layout(location = 0) uniform mat3 transform;

void main()
{
  gl_Position = vec4((transform * vec3(a_position, 1.0f)).xy, 0.5f, 1.0f);
}
