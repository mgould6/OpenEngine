#version 330 core

in vec4 BoneColor;
out vec4 fragColor;  // Renamed from FragColor to prevent redefinition errors

void main() {
    fragColor = BoneColor; 
}
