constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core
in vec3 normal;
in vec3 position;

layout(location = 0) out vec4 g_position;
layout(location = 1) out vec4 g_normal;
layout(location = 2) out vec3 g_stitched_normal;
layout(location = 3) out vec4 g_albedo;

layout(location = 3) uniform vec4 color;
layout(location = 4) uniform bool unlit = false;

void main()
{
  if (color.a == 0.0f)
    discard;

  g_position.xyz = position;
  // 4-th component of position is used for specular strength
  g_position.w = 0.6f;
  g_normal.xyz = normalize(normal);
  // 4-th component of normal is used to determine if pixel should be lit
  g_normal.w = unlit ? 0.0f : 1.0f; 
  g_stitched_normal = vec3(0.0f, 0.0f, 0.0f);
  g_albedo = color;
}

)";