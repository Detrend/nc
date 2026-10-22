
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 a_uv;

out vec3 uv;

layout(location = 0) uniform mat4 transformationMatrix;

layout(location = 1) uniform vec2 atlas_size;
layout(location = 2) uniform vec3 texture_pos;
layout(location = 3) uniform vec2 texture_size;
layout(location = 4) uniform int character;
layout(location = 5) uniform int width;
layout(location = 6) uniform int height;

void main(void) {
	gl_Position = transformationMatrix * vec4(position, 0.0, 1.0);

	float texX = mod(float(character), float(width));
    float texY = floor(float(character) / float(width));
	
	vec2 n_uv = a_uv;

    n_uv.x = n_uv.x / float(width) + texX / float(width);
    n_uv.y = n_uv.y / float(height) + texY / float(height);

	
	uv = vec3((n_uv * texture_size + texture_pos.xy) / atlas_size, texture_pos.z);

}

