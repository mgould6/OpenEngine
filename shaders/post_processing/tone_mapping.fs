#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D hdrBuffer;
uniform float exposure; // Make exposure adjustable from application
uniform float gamma;    // Make gamma adjustable from application

void main() {
    vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure); // Reinhard tone mapping
    mapped = pow(mapped, vec3(1.0 / gamma));             // Gamma correction
    FragColor = vec4(mapped, 1.0);
}
