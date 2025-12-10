constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core
      
in vec2 uv;
      
out vec4 FragColor;

uniform sampler2D sampler;

layout(location = 4) uniform bool hover = false;
layout(location = 5) uniform vec4 color = vec4(1, 1, 1, 1);

void main(void) {
    FragColor = texture(sampler, uv);

    if(hover)
    {
        FragColor.xyz *= 0.5;
    }

    FragColor *= color;
}

)";