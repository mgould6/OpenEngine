#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;

void main() {
    float ssaoFactor = texture(screenTexture, TexCoords).r;
    FragColor = vec4(vec3(ssaoFactor), 1.0); // Debug SSAO output
}
