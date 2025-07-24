#version 330 core
out vec4 FragColor;

in vec3 PosColor;

void main() {
    FragColor = vec4(PosColor, 1.0);
}
