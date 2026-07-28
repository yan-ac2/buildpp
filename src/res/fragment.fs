#version 430 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D tgaTexture; // TGA texture unit bound in C++

void main() {
    FragColor = texture(tgaTexture, TexCoord);
}