constexpr const char* VERTEX_SOURCE = R"(

#version 430 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 a_uv;

out vec2 uv;

layout(location = 0) uniform mat4 transformationMatrix;

layout(location = 1) uniform vec2 atlas_size;
layout(location = 2) uniform vec2 texture_pos;
layout(location = 3) uniform vec2 texture_size;
layout(location = 4) uniform int character;

void main(void) {
	gl_Position = transformationMatrix * vec4(position, 0.0, 1.0);

	float texX = mod(character, 8);
    float texY = character / 8;
	
	vec2 n_uv = a_uv;

    n_uv.x = n_uv.x / 8.0 + texX / 8.0;
    n_uv.y = n_uv.y / 16.0 + texY / 16.0;

	
	uv = (n_uv * texture_size + texture_pos) / atlas_size;

}


)";