
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 a_uv;

out vec3 uv;

layout(location = 0) uniform mat4 transformationMatrix;

layout(location = 1) uniform vec2 atlas_size;
layout(location = 2) uniform vec3 texture_pos;
layout(location = 3) uniform vec2 texture_size;

void main(void) {
	gl_Position = transformationMatrix * vec4(position, 0.0, 1.0);
	uv = vec3((a_uv * texture_size + texture_pos.xy) / atlas_size, texture_pos.z);
}
