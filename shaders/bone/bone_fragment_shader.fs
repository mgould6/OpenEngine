#version 330 core
out vec4 FragColor;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse;

void main() {
    vec3 color = texture(texture_diffuse, TexCoords).rgb;
    FragColor = vec4(color, 1.0);
}
