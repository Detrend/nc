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
layout(location = 5) uniform float width;
layout(location = 6) uniform float height;

void main(void) {
	gl_Position = transformationMatrix * vec4(position, 0.0, 1.0);

	float texX = mod(character, int(width));
    float texY = character / int(width);
	
	vec2 n_uv = a_uv;

    n_uv.x = n_uv.x / width + texX / width;
    n_uv.y = n_uv.y / height + texY / height;

	
	uv = (n_uv * texture_size + texture_pos) / atlas_size;

}


)";