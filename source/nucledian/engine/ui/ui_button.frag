constexpr const char* FRAGMENT_SOURCE = R"(

#version 430 core
      
in vec2 textureCoords;
in vec2 uv;
      
out vec4 FragColor;
uniform sampler2D guiTexture;

void main(void) {
    FragColor = texture(guiTexture, textureCoords);
}

)";