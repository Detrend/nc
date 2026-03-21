constexpr const char* VERTEX_SOURCE = R"(

#version 430 core

layout(location = 0) uniform vec2 u_from;
layout(location = 1) uniform vec2 u_to;
layout(location = 2) uniform vec2 u_megatex_size;
layout(location = 4) uniform vec3 u_wp00;
layout(location = 5) uniform vec3 u_wp10;
layout(location = 6) uniform vec3 u_wp01;
layout(location = 7) uniform vec3 u_wp11;
layout(location = 9) uniform vec3 u_norm;

out vec2 uv;
out vec3 wp;
out vec3 normal;

void main()
{
  // Build a quad from 6 vertices using gl_VertexID
  const vec2 corners[6] = vec2[](
    vec2(0, 0), vec2(1, 0), vec2(1, 1),
    vec2(0, 0), vec2(1, 1), vec2(0, 1)
  );

  vec2 local = corners[gl_VertexID];
  vec2 pixel = mix(u_from, u_to, local);
  vec2 ndc   = (pixel / u_megatex_size) * 2.0 - 1.0;

  vec3 wp_bottom = mix(u_wp00, u_wp10, local.x);
  vec3 wp_top    = mix(u_wp01, u_wp11, local.x);
  wp             = mix(wp_bottom, wp_top, local.y);
  uv             = local;
  normal         = u_norm;
  gl_Position = vec4(ndc, 0.0, 1.0);
}

)";