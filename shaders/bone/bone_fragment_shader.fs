#version 330 core
out vec4 FragColor;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse;
uniform bool useTexture; // Pass as a uniform from the engine

void main() {
    vec3 color;
    if (useTexture) {
        color = texture(texture_diffuse, TexCoords).rgb;
    } else {
        color = vec3(1.0, 1.0, 1.0); // Default to white if no texture
    }
    FragColor = vec4(color, 1.0);
}
