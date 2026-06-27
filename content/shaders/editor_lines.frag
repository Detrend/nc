#version 430 core

layout(location = 1) uniform vec4 color;

out vec4 fragColor;

void main()
{
  fragColor = color;
}
