
#version 430 core
#extension GL_NV_gpu_shader5 : enable

in      vec3 position;
in      vec3 stitched_position;
in      vec3 normal;
in      vec2 uv;
in flat vec3 stitched_shading_position;
in flat vec3 shading_position;

layout(location = 0) out vec4 g_position;
layout(location = 1) out vec4 g_stitched_position;
layout(location = 2) out vec4 g_normal;
layout(location = 3) out vec4 g_stitched_normal;
layout(location = 4) out vec4 g_albedo;
layout(location = 5) out uint g_sector;

layout(binding = 0) uniform sampler2D sampler;

layout(location = 6) uniform uint sector_id;
layout(location = 8) uniform uint matrix_id;
layout(location = 9) uniform bool enable_shadows;

void main()
{
  vec4 color = texture(sampler, uv);
  if (color.a < 0.95f)
    discard;

  g_position.xyz = position;
  // 4-th component of position is used for specular strength
  g_position.w = 0.0f;
  g_stitched_position = vec4(stitched_position, uintBitsToFloat(matrix_id));
  // 4-th component of normal is used to determine if pixel is a billboard
  // First 3 components are used as shading pos
  g_normal = vec4(shading_position, 0.0f);
  // First 3 components are used as stitched shading pos
  g_stitched_normal.xyz = stitched_shading_position; // store the stitched shading position here
  // 4-th component of stitched_normal is used to determine if shadows are enabled
  g_stitched_normal.w = enable_shadows ? 1.0f: 0.0f;
  g_albedo = color;
  g_sector = sector_id;
}
