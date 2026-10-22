    
in vec3 uv;
      
layout(location = 7) uniform bool hover;

out vec4 FragColor;

uniform mediump sampler2DArray  sampler;

void main(void) {
    FragColor = texture(sampler, uv);
    if(hover)
    {
        FragColor.xyz *= 0.5;
    }
}
