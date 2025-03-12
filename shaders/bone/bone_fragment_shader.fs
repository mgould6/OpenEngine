#version 330 core

in vec4 BoneColor;
out vec4 FragColor;

void main() {
    FragColor = BoneColor;
}

out vec4 FragColor;

uniform sampler2D texture_diffuse;
uniform bool useTexture;

void main()
{
    vec3 color;
    if (useTexture) {
        color = texture(texture_diffuse, TexCoords).rgb;
    } else {
        color = vec3(1.0);
    }
    FragColor = vec4(color, 1.0);
}
