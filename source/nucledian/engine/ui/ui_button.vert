constexpr const char* VERTEX_SOURCE = R"(

#version 430 core

layout(location = 0) in vec2 position;

layout(location = 0) uniform mat4 transformationMatrix;

void main(void) {
	gl_Position = transformationMatrix * vec4(position, 0.0, 1.0);
}


)";