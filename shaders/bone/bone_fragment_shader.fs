#version 330 core
out vec4 FragColor;

in vec3 BoneColor;  // NEW: from vertex shader

void main() {
    FragColor = vec4(BoneColor, 1.0);  // Use bone ID color
}
