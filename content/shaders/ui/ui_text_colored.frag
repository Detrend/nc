in vec2 uv;

out vec4 FragColor;

uniform sampler2D sampler;

// Text with a tint and alpha. Pairs with ui_text.vert (uses locations 0-6).
layout(location = 8) uniform vec4 color;

void main(void) {
    FragColor = texture(sampler, uv) * color;
}
