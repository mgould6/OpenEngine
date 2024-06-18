// bright_extract.fs
#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;
uniform float exposure;

void main()
{
    vec3 color = texture(scene, TexCoords).rgb;
    vec3 bright = max(color - vec3(exposure), 0.0);
    FragColor = vec4(bright, 1.0);
}
