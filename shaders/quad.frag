#version 430 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uScreenTex;

void main() {
    FragColor = texture(uScreenTex, vTexCoord);
}
