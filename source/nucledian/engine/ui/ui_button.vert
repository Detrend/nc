constexpr const char* VERTEX_SOURCE = R"(

#version 430 core

in vec2 position;

uniform mat4 transformationMatrix;

void main(void) {
	gl_Position = transformationMatrix * vec4(position, 0.0, 1.0);
}


)";