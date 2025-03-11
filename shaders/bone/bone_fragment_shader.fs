#version 330 core

in vec3 Normal;
in vec2 TexCoords;

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
