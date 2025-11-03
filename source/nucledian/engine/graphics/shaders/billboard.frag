constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core
in vec3 position;
in vec2 uv;

layout(location = 0) out vec4 g_position;
layout(location = 1) out vec4 g_normal;
layout(location = 2) out vec3 g_stitched_normal;
layout(location = 3) out vec4 g_albedo;

layout(binding = 0) uniform sampler2D sampler;

void main()
{
  vec4 color = texture(sampler, uv);
  if (color.a < 0.95f)
    discard;

  g_position.xyz = position;
  // 4-th component of position is used for specular strength
  g_position.w = 0.0f;
  // 4-th component of normal is used to determine if pixel should be lit
  g_normal = vec4(0.0f, 0.0f, 0.0f, 0.0f);
  g_stitched_normal = vec3(0.0f, 0.0f, 0.0f);
  g_albedo = color;
}

)";