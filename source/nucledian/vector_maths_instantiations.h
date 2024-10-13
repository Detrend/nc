// DO NOT INCLUDE BY ITSELF!!!

//==============================================================================
template bool eq<f32, 4>(const vec4&, const vec4&);
template bool eq<f32, 3>(const vec3&, const vec3&);
template bool eq<f32, 2>(const vec2&, const vec2&);

template vec4 min<f32, 4>(const vec4&, const vec4&);
template vec3 min<f32, 3>(const vec3&, const vec3&);
template vec2 min<f32, 2>(const vec2&, const vec2&);

template vec4 max<f32, 4>(const vec4&, const vec4&);
template vec3 max<f32, 3>(const vec3&, const vec3&);
template vec2 max<f32, 2>(const vec2&, const vec2&);

template vec4 add<f32, 4>(const vec4&, const vec4&);
template vec3 add<f32, 3>(const vec3&, const vec3&);
template vec2 add<f32, 2>(const vec2&, const vec2&);

template vec4 sub<f32, 4>(const vec4&, const vec4&);
template vec3 sub<f32, 3>(const vec3&, const vec3&);
template vec2 sub<f32, 2>(const vec2&, const vec2&);

template vec4 mul<f32, 4>(const vec4&, const vec4&);
template vec3 mul<f32, 3>(const vec3&, const vec3&);
template vec2 mul<f32, 2>(const vec2&, const vec2&);

template vec4 div<f32, 4>(const vec4&, const vec4&);
template vec3 div<f32, 3>(const vec3&, const vec3&);
template vec2 div<f32, 2>(const vec2&, const vec2&);

template f32  dot<f32, 4>(const vec4&, const vec4&);
template f32  dot<f32, 3>(const vec3&, const vec3&);
template f32  dot<f32, 2>(const vec2&, const vec2&);
//==============================================================================

